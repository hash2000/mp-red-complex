# FetchApi

`FetchApi` — модуль выполнения REST-запросов: аналог Postman внутри приложения.
Он даёт документ с редактором запроса и просмотром ответа, консольную команду
`fetch-api` и сервис, которым могут пользоваться другие модули.

Каталог модуля: `src/Content/FetchApiModule/`.

Все начальные данные запроса берутся из **переменных контекста**, а результат
запроса записывается обратно в контекст. Это позволяет готовить запрос
командой, открывать документ с уже заполненными полями и связывать FetchApi
с другими документами через `CommandContext`.

## 1. Состав модуля

| Часть | Назначение |
|---|---|
| `models/fetch_api_types.h` | HTTP-метод, тип обработчика, политика TLS, имена переменных, разбор пар «имя → значение» |
| `models/fetch_opt.h` | `FetchApiOpt` — параметры запроса и сборка `QNetworkRequest` |
| `models/fetch_api_result.h` | `FetchApiResult` — результат запроса и запись его в контекст |
| `services/fetch_service.h` | `FetchApiService` — выполнение запросов, TLS, тайм-ауты, ошибки |
| `documents/fetch_api_document.h` | документ: связывает виджет с контекстом |
| `documents/fetch_api_document_metadata.h` | метаданные документа для диалога создания |
| `widgets/fetch_api_widget.h` | самостоятельный виджет-редактор (не знает о контексте); ответ — `QPlainTextEdit` |
| `commands/fetch_api_cmd.h` | консольная команда `fetch-api` |
| `tests/test_fetch_api.cpp` | `TestFetchApi` |

Слои не перемешиваются: модель не знает о Qt-сети, сервис — о контекстах,
виджет — о документе. Контекст связывает всё через документ и команду.
Модуль не зависит от `CodeEditorWidget`: тело ответа показывается в простом
текстовом поле (`QPlainTextEdit`).

## 2. `FetchApiOpt` — параметры запроса

```cpp
FetchApiOpt options("https://api.example.com/v1/items");
options.setMethod(FetchApiMethod::Post);            // либо setMethod("post")
options.setBodyText(R"({"name":"x"})");
options.setParametersFormatted("page=2; size=10");  // query
options.setHeadersFormatted("X-Test: 1");
options.setSslPolicy(FetchApiSslPolicy::TrustCustomCa);
options.loadCustomCaFromFile("C:/certs/ca.pem");
options.setHandler(FetchApiHandlerType::Rest);
options.setTimeoutMs(15000);                        // 0 — по умолчанию сервиса

QNetworkRequest request = options.request();
```

`request()` каждый раз собирает новое представление:

- адрес без схемы получает `https://`;
- query-параметры добавляются через `QUrlQuery` (пустые значения сохраняются);
- заголовки переносятся как raw headers;
- если тело непустое и `Content-Type` не задан, ставится `application/json`
  для JSON-тела и `text/plain; charset=utf-8` для остального;
- при политике `TrustCustomCa` в SSL-конфигурацию запроса подставляются только
  переданные CA-сертификаты;
- `timeoutMs > 0` превращается в `QNetworkRequest::setTransferTimeout()`.

Вспомогательный разбор пар понимает три формы:

```text
Content-Type: application/json      # по строке на пару
a=1; b=2                            # через ;
?page=2&size=10                     # query-строка
```

## 3. `FetchApiResult` — результат

Результат один и для успеха, и для ошибки, поэтому у вызывающей стороны нет
двух веток:

| Поле | Значение |
|---|---|
| `failed` | сеть, TLS или HTTP >= 400 |
| `statusCode`, `statusText` | код и причина HTTP; 0 — ответа нет |
| `data` | тело ответа (`QByteArray`) |
| `headers` | заголовки ответа (`QHttpHeaders`) |
| `errorCode`, `errorString` | ошибка транспорта и текст для пользователя |
| `elapsedMs`, `size` | время и размер |
| `url` | фактический адрес (после редиректов) |

`isSuccess()` — 2xx без ошибки транспорта, `isHttpError()` — код >= 400.
`text()`, `contentType()`, `headersFormatted()` и `cookies()` — готовые
представления для интерфейса и контекста.

## 4. Переменные контекста

### Входные параметры запроса

| Переменная | Назначение |
|---|---|
| `location` (алиас `url`) | адрес запроса |
| `method` | `GET`, `POST`, `PUT`, `DELETE`, `PATCH`, `HEAD` (регистр не важен) |
| `body` | тело: строка, `<<text>>…<</text>>` или `<<json>>…<</json>>` |
| `params` (алиас `query`) | query-параметры в формате пар |
| `headers` | заголовки запроса в формате пар |
| `ssl-policy` | `strict`, `trust-custom-ca`, `ignore-errors` |
| `ssl-ca-file` | файл CA-сертификата (PEM/DER) для `trust-custom-ca` |
| `handler` | тип обработчика; сейчас только `rest` |
| `content-type` | необязательный `Content-Type` запроса, если его нет в `headers` |
| `show-editor` | `true` — открыть тело ответа в code-editor через команду `documents` (необязательная функция; сам модуль от редактора не зависит) |

### Результат запроса

| Переменная | Назначение |
|---|---|
| `data` | контент ответа строкой (mime-тип — из `content-type`) |
| `content-type` | `Content-Type` ответа |
| `headers` | заголовки ответа, по строке на заголовок |
| `status-code` | код HTTP |
| `status-text` | причина HTTP («OK», «Not Found») |
| `error` | текст ошибки; пустая строка при успехе |
| `error-code` | код ошибки транспорта (0 при успехе) |
| `elapsed-ms` | время выполнения, мс |
| `size` | размер тела, байт |
| `url` | фактический адрес ответа |
| `method` | метод выполненного запроса |

После записи результата документ и команда отправляют событие
**`fetch-api.complete`** — на него можно подписаться в соседнем документе.

> **`headers` до и после запроса.** До запроса это заголовки запроса, после
> запроса — заголовки ответа (требование модуля). Документ читает `headers`
> как заголовки запроса только при привязке контекста, чтобы не подменять
> редактор ответом.

## 5. `FetchApiService`

```cpp
auto* service = controller->services()->get<FetchApiService>();
const bool started = service->execute(options,
    [](const FetchApiResult& result) { /* результат */ },
    [](qint64 received, qint64 total) { /* прогресс */ });
if (!started) {
    controller->printError(service->lastError());
}
```

Сервис принимает `Resources*` (значения по умолчанию) и создаёт
`QNetworkAccessManager` своим дочерним объектом. Владелец сервиса —
`ServicesRegistry`, поэтому родитель сервису не назначается.

Возможности:

- все методы `FetchApiMethod`; для `HEAD` используется `head()`, для
  `PUT`/`DELETE`/`PATCH` — `sendCustomRequest()`;
- политики TLS: `Strict` (ошибка не подавляется), `TrustCustomCa`
  (подавляются только ошибки для сертификатов из набора `FetchApiOpt`),
  `IgnoreErrors` (`ignoreSslErrors()`);
- тайм-аут из `fetch-api.timeoutMs` (по умолчанию 30000 мс), если модель его
  не задала;
- `User-Agent` из `fetch-api.userAgent` (по умолчанию `RedComplex-FetchApi/1.0`);
- `lastError()` — причина синхронного отказа (пустой адрес, неизвестный
  обработчик, `trust-custom-ca` без сертификата);
- `pendingRequests()` и `cancelAll()` — для завершения работы модуля.

Сигналы сервиса: `requestStarted`, `requestFinished`, `requestFailed`,
`errorOccurred`, `downloadProgress`.

Ошибки «по общему принципу»: сервис заполняет `FetchApiResult` и `lastError()`,
команда и документ печатают их через `CommandController::printError()`.
HTTP-ответы 4xx/5xx считаются неуспешными, но тело и заголовки сохраняются.

## 6. Команда `fetch-api`

```text
fetch-api action=send, location="https://example.com/api"
fetch-api action=send, location="https://example.com/api", method=post,
          headers="Content-Type: application/json", body=<<json>>{"x":1}<</json>>
fetch-api action=get, location="https://example.com/api"
fetch-api action=send, location="https://self-signed.local", ssl-policy=trust-custom-ca,
          ssl-ca-file="C:/certs/ca.pem"
fetch-api action=show-ssl
```

- `action=send` — отправить запрос; `action=<метод>` — сокращённая запись
  (`action=get`, `action=post`, ...);
- `action=show-ssl` — таблица с поддержкой TLS, бэкендом, версиями OpenSSL и
  числом системных CA;
- результат пишется в ближайший документ-контекст, а если команду вызвали из
  консоли — в глобальный контекст, поэтому значения видны следующим командам;
- `show-editor=true` открывает ответ в code-editor через штатную команду
  `documents` (документ получает переменную `data`); это отдельная функция
  системы команд, а не зависимость модуля от `CodeEditorWidget`;
- при ошибке команда печатает `printError()` и не прерывает консоль.

## 7. Документ и виджет

`FetchApiWidget` — самостоятельный редактор: он не знает о документе и его
состоянии. Он собирает `FetchApiOpt` из полей, выполняет запрос через сервис и
сообщает о завершении сигналами `requestStarted`, `requestFinished`,
`requestFailed`. В самом виджете остаются метод, адрес и вкладки запроса;
отправка и её параметры вынесены в документ. Ответ выводится в
`QPlainTextEdit` — модуль не использует `CodeEditorWidget`.

`FetchApiDocument` — документ с контекстом:

1. сверху лента (`DocumentRibbon`) с кнопкой **«Отправить»**; её параметры
   редактируются в панели «Свойства»;
2. при привязке контекста читает входные переменные и заполняет редактор;
3. перед отправкой записывает параметры редактора в контекст (`location`,
   `method`, `body`, `params`, `ssl-policy`, `handler`);
4. после ответа вызывает `FetchApiResult::applyToContext()` (`data`,
   `content-type`, `headers`, `status-code`, ...) и отправляет
   `fetch-api.complete`;
5. ошибки печатает в консоль, панель свойств показывает статус, время и размер.

Параметры кнопки «Отправить» — группа свойств `request`
(«Параметры отправки»):

| Свойство | Значение |
|---|---|
| `handler` — «Тип запроса» | обработчик запроса; сейчас только `REST` |
| `ssl-policy` — «Политика TLS» | `Strict`, `Trust custom CA`, `Ignore errors` |
| `ssl-ca-file` — «Файл CA-сертификата» | `PEM`/`DER`-файл для `Trust custom CA` |

Значок свойств в правом нижнем углу кнопки «Отправить» раскрывает эту группу
в панели «Свойства» (`showProperties("request")`). Изменения сразу попадают
в переменные контекста (`handler`, `ssl-policy`, `ssl-ca-file`), поэтому
запрос можно восстановить после перезапуска.

Возможности редактора:

- метод и адрес, тип обработчика — в свойствах;
- вкладки `Body`, `Headers`, `Params`, `Auth`;
- `Auth`: без авторизации, `Basic` (`Authorization: Basic base64(логин:пароль)`),
  `Bearer token` (`Authorization: Bearer <токен>`), `API key`
  (заголовок или query-параметр);
- политика TLS и файл CA-сертификата для `Trust custom CA` — в свойствах;
- ответ: `Body` (простое текстовое поле без подсветки; JSON показывается
  с отступами), `Headers`, `Cookies` (разобранные `Set-Cookie`), статус, время,
  размер и индикатор загрузки.

Документ создаётся командой:

```text
documents action=create, target=fetch-api, id=api,
          location="https://api.example.com/v1/items", method=get
```

## 8. Типы обработчиков

`FetchApiHandlerType` сейчас содержит только `Rest`. Строковые имена —
`Format<FetchApiHandlerType>` / `From<FetchApiHandlerType>` (`rest`). Новый вид
(gRPC и т. п.) добавляется значением перечисления и веткой в
`FetchApiService::execute()`; модель, команда и документ менять не требуется.

## 9. Оформление

Стандартные глиф и цвет вкладки задаёт тема:
`assets/themes/{theme}/documents.yaml` — `Content.FetchApi.FetchApiDocument` и
`fetch-api`. Стили контролов — `assets/themes/{theme}/stylesheets/fetch-api.qss`;
лента и значок свойств оформляются общими правилами `base.qss`. Динамическое
состояние метки статуса и индикатора — свойство `state` (`success` / `error`).

## 10. Интеграция

- `ServicesFactory` (`src/Launcher/services.cpp`):
  `registerFactory<FetchApiService>([]{ return std::make_unique<FetchApiService>(resources); })`;
- `WidgetsBuilder` (`src/Launcher/widgets_builder.cpp`) регистрирует
  `Content.FetchApi.FetchApiDocument` / `fetch-api` и метаданные документа;
- команда регистрируется в `ApplicationController::initContext()`
  как `FetchApiCommand`;
- цель сборки — `RedComplexFetchApiModule`, тест — `TestFetchApi`.

## 11. Тесты

`TestFetchApi` (`tests/test_fetch_api.cpp`) проверяет разбор пар, преобразования
методов/политик/обработчиков, сборку запроса, запись результата в контекст,
валидацию сервиса, реальный HTTP-обмен с локальным сервером, а также редактор:
параметры, авторизацию и отображение ответа. Для документа проверяются группа
свойств «Параметры отправки» (тип запроса, TLS, файл CA), запись настроек в
контекст и кнопка ленты «Отправить». Отдельно проверяется, что тело ответа —
`QPlainTextEdit`, а не `CodeEditorWidget`.

## 12. См. также

- [Контексты](../contexts/README.md)
- [Документы](../documents/README.md)
- [Команды консоли](../console/commands.md)
- [SystemRouter](../system-router/README.md)
