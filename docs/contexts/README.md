# Контексты

`CommandContext` — узел дерева контекстов приложения. Через него команды,
документы и панели обмениваются переменными и событиями, не зная друг о друге
напрямую. Реализация: `src/Libs/Engine/scripting/command_context.*`.

## 1. Дерево контекстов

- Корневой контекст создаёт `CommandController`; его namespace — `.`.
- `createScopedContext(name)` создаёт дочерний контекст и регистрирует его в
  namespace глобального контекста. Повторные вызовы дают разные узлы, если
  namespace ещё не занят; для поиска используйте `context(ns)`.
- Namespace вложенного контекста имеет вид `.родитель.ребёнок`, например
  `.store.system_router`.
- Владелец контекста хранит `std::shared_ptr`. Ссылки на детей в реестре —
  слабые: если родитель не держит `shared_ptr`, узел может исчезнуть.
- Контексты команд, создаваемые `CommandProcessor`, **не регистрируются** в
  глобальном дереве и живут только на время вызова команды.

```cpp
auto store = controller->globalContext()->createScopedContext("store");
auto systemRouter = store->createScopedContext("system_router");

auto found = controller->globalContext()->context(".store.system_router");
```

Для одноразовой работы создавайте дочерний контекст и сохраняйте его в
`shared_ptr` на всё время операции. Не сохраняйте `CommandContext*` дольше,
чем живёт владелец.

## 2. Режимы переменных

Режим задаётся `setVariablesMode()` и может меняться в любой момент.

| Режим | Чтение | Запись |
|---|---|---|
| `Lazy` | читает значение базового контекста | при записи создаёт собственную копию; база не меняется |
| `Mutable` | читает базовое значение | пишет общую переменную базового контекста |
| `ReadOnly` | читает базовое значение | запрещена, `setVariable()` возвращает `false` |
| `Atomic` | при переключении получает копию видимых переменных | дальше живёт изолированно |

Собственные переменные контекста изолированы в любом режиме. Именно поэтому
контекст документа обычно работает как `Lazy`: он видит переменные создателя,
но его изменения не затирают контекст-создатель.

Контекст команды создаётся с режимом `Mutable`: команда видит переменные
вызывающей стороны и может писать в них. Параметры командной строки
записываются в контекст команды **до** переключения в `Mutable`, поэтому они
остаются собственными переменными команды.

## 3. Переменные

```cpp
context->setVariable("count", 3);
context->setVariableMimeType("count", "application/x-integer");

const int count = context->variable("count", 0).toInt();
const bool hasPath = context->hasVariable("path");

const QVariantMap visible = context->variables();   // базовые + собственные
const QVariantMap own = context->ownVariables();    // только собственные
const QStringList names = context->variableNames();
```

Правила:

- имя переменной — произвольная строка; точки допустимы (`wal.checkInterval`);
- `variable()` не создаёт переменную, а лишь возвращает значение или
  `defaultValue`; для документа-обёртки `DocumentWidget::variable()` создаёт
  переменную с дефолтом при отсутствии;
- `variables()` возвращает видимые переменные, `ownVariables()` — только
  собственные;
- `setVariable()` возвращает `false` в режиме `ReadOnly`;
- mime-тип переменной — отдельное строковое значение:
  `setVariableMimeType()` / `variableMimeType()`. Оно не влияет на хранение
  `QVariant`, но используется командами и диалогами для выбора редактора.

### Mime-типы переменных

Основные значения:

| Mime-тип | Назначение |
|---|---|
| `text/plain` | обычный текст |
| `text/x-multiline` | многострочный текст |
| `text/x-file-path` | путь к файлу |
| `text/x-directory-path` | путь к каталогу |
| `application/x-boolean` | флаг |
| `application/x-integer` | целое |
| `application/x-number` | число |
| `application/x-color` | цвет |
| `application/json`, `application/xml`, `text/x-ini` | структурированные данные |
| `application/x-mp-shared` | переменную можно передать в контекст создаваемого документа |
| `application/x-mp-context-variable` | значение свойства можно передать в контекст команды |

`application/x-mp-shared` не делает переменную видимой сама по себе: при
создании документа `WidgetZoneCommand` копирует в контекст-создатель только
переменные с этим mime-типом. В SystemRouter, чтобы свойство узла дошло до
команды и затем до созданного документа, ему нужны одновременно
`application/x-mp-context-variable` и `application/x-mp-shared`.

Mime-тип может быть списком; в контексте он хранится строкой, поэтому для
списка используется строка с разделителями (как правило, `", "`). Не
рассчитывайте на точное равенство строки, если проверяете отдельный токен:
разбирайте список.

## 4. События

```cpp
context->fireEvent("document.save.complete");
```

- `fireEvent(name)` испускает `eventFired(name)`; на него подписываются
  документы и другие заинтересованные объекты.
- `CommandContext::documentSaveCompleteEvent()` — имя
  `document.save.complete`, отправляется документом после успешного сохранения.
- `CommandContext::variablesBatchCompleteEvent()` — имя
  `variables.batch.complete`, отправляется после пакетного изменения.

Пакетное изменение:

```cpp
context->beginVariablesBatch();
context->setVariable("a", 1);
context->setVariable("b", 2);
context->endVariablesBatch();
// отправлено eventFired("variables.batch.complete"),
// changedVariables() содержит {"a", "b"} до завершения пакета
```

Во время пакета отдельные `variableChanged()` не рассылаются. Это удобно,
когда несколько переменных должны обновиться атомарно.

## 5. Контексты документов

Для каждого контекста существует ленивый дочерний контекст документов:

```cpp
auto docs = context->documentsContext();
auto documentContext = context->createDocumentContext("notes");
auto existing = context->documentContext("notes");
```

- Контекст документа имеет вид `{documents}.{documentId}`.
- Если документ создан документом, полный путь получается вложенным:
  `.documents.{creator}.documents.{id}`.
- `isDocumentContext()` и `documentId()` позволяют определить, является ли
  контекст контекстом документа.
- `owningDocumentContext()` возвращает ближайший документ-контекст. Это
  используется командами и панелями, чтобы понять, из какого документа
  пришёл вызов.

Сигналы в контексте документов:

| Сигнал | Когда возникает |
|---|---|
| `documentAttached(id)` | в контексте документов появился новый документ |
| `documentActivated(id)` | документ стал активным |
| `documentDeactivated()` | активный документ перестал быть активным |
| `documentInvoked(id)` | существующий документ был вызван повторно |

## 6. Время жизни и очистка

- Контексты хранятся через `std::shared_ptr`; дерево не является владельцем
  документов.
- `pruneDeadContexts()` удаляет из реестров записи, чьи владельцы уже
  уничтожены, и возвращает количество удалённых.
- `CommandController::init()` запускает фоновую очистку. Период задаётся
  переменной `command.context.cleanupIntervalMs` (по умолчанию 30000 мс).
- Если создаёте временный контекст, держите его в `shared_ptr` до завершения
  асинхронной операции; иначе он может быть удалён очисткой.

## 7. Практические правила

1. Не пишите в чужие контексты напрямую, если можно отправить событие или
   изменить свою переменную.
2. Для команды используйте `context->variable("name", default)`, а не
   глобальные переменные.
3. Проверяйте `setVariable()` в контекстах, которые могут быть `ReadOnly`.
4. Для временных вычислений создавайте дочерний контекст, а не меняйте
   глобальный.
5. Не храните `CommandContext*` без `shared_ptr`.
6. При добавлении нового mime-типа обновите и таблицу редакторов, и места, где
   проверяются токены mime-типов.

## См. также

- [Документы](../documents/README.md)
- [Команды консоли](../console/README.md)
- [Узлы SystemRouter](../system-router/nodes.md)
