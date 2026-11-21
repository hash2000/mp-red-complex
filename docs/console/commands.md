# Встроенные команды консоли

Ниже перечислены команды, зарегистрированные в
`ApplicationController::initContext()` и модулях. Синтаксис общий:
`command name=value, name2=value2`.

## `help`

```text
help
help name=documents
help name=system-router-routes
```

- без параметров — таблица команд и глобальных переменных;
- с параметром — справка по команде или переменной. В коде используется
  значение первого параметра, поэтому имя параметра не важно; в примерах
  используется `name`.

## `echo`

```text
echo name="Привет", count=3, enabled=true
```

Выводит таблицу `Name | mime-type | text` для всех параметров вызова.

Поддерживаются блоки:

```text
echo data=<<json>>{"a":1,"b":[2,3]}<</json>>
echo text=<<text>>первая строка
вторая строка<</text>>
```

`echo` — удобный способ проверить, как парсер разобрал значения и какие
mime-типы он назначил.

## `documents`, `info-panels`, `side-panels`

Это три экземпляра `WidgetZoneCommand`: для документов, нижних и боковых
панелей соответственно.

```text
documents action=create, target=system-router, id=router
documents action=create, target=code-editor, id=notes, path="C:/data/notes.txt"
documents action=show, id=router
documents action=close, id=notes
documents action=list
```

Параметры `create`:

| Параметр | Обязательность | Назначение |
|---|---|---|
| `target` | да | xtype или полное имя класса из `WidgetRegistry` |
| `id` | нет | ID документа; без него генерируется UUID |
| `name`, `title`, `glyph`, `tooltip` | нет | параметры фабрики/вкладки |
| остальные | нет | начальные переменные контекста документа |

Особенности:

- повторный `id` в той же зоне активирует существующий виджет;
- `id`, занятый другой зоной, — ошибка;
- синглтон всегда получает `id = xtype`;
- переменные с mime-типом `application/x-mp-shared` из контекста команды
  копируются в контекст-создатель, поэтому новый документ их видит.

`action=show`, `action=close` и `action=list` используют только `id`
(`list` не принимает параметров); `list` выводит таблицу `Id | Title | Type`.

## `users`

```text
users action=login, username=admin, password="секрет"
users action=logout
users action=register, username=admin, password="секрет"
users action=restore, seed="слово1 слово2 ...", password="новый пароль"
```

Команда работает с `UsersService` и `UsersRegistryService`; результат
выводится таблицей/сообщениями в консоль.

## `fetch-api`

Модуль FetchApi, аналог Postman: запрос собирается из переменных контекста,
ответ записывается в переменные контекста. Подробности —
[docs/fetch-api](../fetch-api/README.md).

```text
fetch-api action=show-ssl
fetch-api action=send, location="https://example.com/api"
fetch-api action=get, location="https://example.com/api"
fetch-api action=post, location="https://example.com/api",
          headers="Content-Type: application/json",
          body=<<json>>{"x":1}<</json>>, show-editor=true
fetch-api action=send, location="https://self-signed.local",
          ssl-policy=trust-custom-ca, ssl-ca-file="C:/certs/ca.pem"
```

Параметры:

| Параметр | Назначение |
|---|---|
| `action` | `send` (отправить), HTTP-метод как сокращение (`get`, `post`, ...) либо `show-ssl` |
| `location` (алиас `url`) | адрес запроса |
| `method` | `get`, `post`, `put`, `delete`, `patch`, `head`; по умолчанию `get` |
| `params` (алиас `query`) | query-параметры: `a=1; b=2` |
| `headers` | заголовки запроса: `Name: value; Name2: value2` |
| `body` | тело запроса; для JSON используйте `<<json>>...<</json>>` |
| `ssl-policy` | `strict`, `trust-custom-ca`, `ignore-errors` |
| `ssl-ca-file` | файл CA-сертификата (PEM/DER) для `trust-custom-ca` |
| `handler` | тип обработчика; сейчас только `rest` |
| `content-type` | `Content-Type` запроса, если его нет в `headers` |
| `show-editor` | `true` — открыть тело ответа в code-editor |

Результат запроса пишется в контекст: `data`, `content-type`, `headers`,
`status-code`, `status-text`, `error`, `error-code`, `elapsed-ms`, `size`,
`url`, `method`. После завершения отправляется событие
`fetch-api.complete`. Из консоли результат попадает в глобальный контекст,
из документа — в контекст документа.

## `system-router-save-blob`

```text
system-router-save-blob
```

Служебная команда, обычно вызываемая из документа свойств SystemRouter по
событию `document.save.complete`. Не принимает параметров: читает переменные
`blobId` и `data` из контекста, в котором вызвана, и сохраняет `data` в
`data_blobs`.

В обычной консольной сессии этих переменных нет, поэтому команда вернёт
ошибку. Используйте её через контекст code-editor или свойства узла.

## `system-router-blobs`

```text
system-router-blobs
```

Выводит таблицу всех записей `data_blobs`:

- `id` — GUID блока;
- `version` — версия;
- `description` — описание;
- `data` — содержимое.

Команда ничего не меняет.

## `system-router-routes`

```text
system-router-routes
```

Выводит плоскую таблицу всех узлов `nodes`:

- `id`;
- `parent_id`;
- `name`;
- `data` — JSON свойств узла (mime + `value`/`value_blob`).

Команда обходит дерево рекурсивно, включая лениво загруженные узлы.

## См. также

- [Синтаксис и разработка команд](README.md)
- [Узлы SystemRouter](../system-router/nodes.md)
- [Скрипты SystemRouter](../system-router/scripts.md)
