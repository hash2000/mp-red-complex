# SystemRouter

`SystemRouter` — модуль системного маршрутизатора: дерево узлов, привязка
команд к узлам, выполнение скриптов и хранение крупных значений в отдельных
блоках.

Каталог модуля: `src/Content/SystemRouter/`.

## 1. Состав модуля

| Часть | Назначение |
|---|---|
| `models/` | `SystemRouterDataNode`, mime-типы, команды и preview-скрипты |
| `data_providers/` | доступ к таблицам `nodes` и `data_blobs` |
| `services/` | `SystemRouterService`, `SystemRouterCommandService`, `SystemRouterBlobService` |
| `documents/` | дерево, свойства узла, выполнение скриптов |
| `widgets/` | `SystemRouterTreeWidget`, диалог выбора скрипта |
| `commands/` | консольные команды модуля |
| `tests/` | `TestSystemRouter`, `TestSystemRouterCommandsDb` |

## 2. Хранилище

База: `system_router`. Таблицы:

```sql
nodes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER REFERENCES nodes(id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    data TEXT NOT NULL DEFAULT '{}'
)

data_blobs (
    id TEXT PRIMARY KEY,
    version INTEGER NOT NULL DEFAULT 1,
    description TEXT NOT NULL DEFAULT '',
    data TEXT NOT NULL DEFAULT ''
)
```

Миграции: `src/Configurations/stores/migrations/system_router/`.

- `20261120090000` — создание `nodes` и `commands`;
- `20261120130000` — переименование `commands` → `data_blobs`,
  `script` → `data`.

Если используется существующий профиль, в его `bootstrap.yaml` должна быть
запись о миграции `20261120130000`, иначе таблица `data_blobs` не появится.

## 3. Узлы

Подробно: [nodes.md](nodes.md).

Кратко:

- узел — запись `nodes`; имя уникально среди соседей без учёта регистра;
- свойства узла хранятся в `nodes.data` как JSON;
- крупные значения и параметры команд выносятся в `data_blobs`;
- для команды в свойстве хранится GUID, а скрипт лежит в `data_blobs`.

Провайдер узлов — `SystemRouterDataProviderDb`; он создаёт
`SystemRouterDataNode`, который решает, какие значения писать в `value_blob`.

## 4. Дерево

`SystemRouterTreeDocument` содержит `SystemRouterTreeWidget` — наследник
`LazyTreeWidget`/`LazyTreeView`.

Возможности:

- добавление, переименование, удаление, перемещение узлов;
- контекстное меню «Свойства…»;
- двойной щелчок:
  - одна команда — выполнить;
  - несколько команд — диалог выбора;
  - команд нет — развернуть/свернуть узел;
- выбор узла сохраняется в переменной контекста `selectedNodeId`.

Перетаскивание выполняет `LazyTreeView::moveNode()`: сначала сохраняется новый
`parent_id`, затем строка переносится в модели. Перемещение внутрь собственного
поддерева запрещено.

## 5. Свойства узла

`SystemRouterNodePropertiesDocument` показывает свойства выбранного узла в
`PropertiesListWidget`:

- добавить/удалить свойство;
- изменить набор mime-типов;
- изменить значение;
- открыть code-editor для свойств-блоков.

Изменения сохраняются сразу, отдельной кнопки «Сохранить» нет.

Свойства с маркерами `application/x-mp-command`,
`application/x-mp-command-parameter` или `application/x-mp-context-variable`
отображаются кнопкой и открывают code-editor. Для свойства-команды
редактируется скрипт команды, для параметра/контекстной переменной — значение
блока.

## 6. Скрипты

Подробно: [scripts.md](scripts.md).

- `application/x-mp-command` — GUID команды; имя свойства используется как
  название скрипта в интерфейсе;
- `application/x-mp-command-parameter` — значение для `{{имя}}` в тексте
  скрипта;
- `application/x-mp-context-variable` — переменная, передаваемая в
  `CommandContext` при выполнении и не участвующая в подстановке;
- `application/x-mp-shared` — разрешает передать переменную в контекст
  создаваемого документа.

## 7. Блоки данных

`data_blobs` используется и для команд, и для значений свойств. В JSON
свойства блок обозначается `value_blob`:

```json
{
  "script": {
    "mime": ["application/x-mp-command-parameter", "text/x-multiline"],
    "value_blob": "11111111-1111-1111-1111-111111111111"
  }
}
```

Если в JSON есть одновременно `value` и `value_blob`, при чтении побеждает
`value`.

Сервис: `SystemRouterBlobService` — чтение, создание, сохранение и удаление
блоков.

## 8. Консольные команды

```text
system-router-blobs
system-router-routes
system-router-save-blob
```

- `system-router-blobs` — таблица всех `data_blobs`;
- `system-router-routes` — таблица всех `nodes`;
- `system-router-save-blob` — служебное сохранение `data` по `blobId` из
  контекста code-editor.

## 9. Интеграция

- `SystemRouterService` — операции над узлами;
- `SystemRouterCommandService` — CRUD команд и сборка `previewsForNode()`;
- `SystemRouterBlobService` — блоки;
- `SystemRouterDataProviderDb`, `SystemRouterCommandsProviderDb` — доступ к БД;
- `WidgetsBuilder` регистрирует `system-router`,
  `system-router-node-properties` и `code-editor`;
- `ApplicationController` регистрирует команды модуля.

## 10. См. также

- [Узлы](nodes.md)
- [Скрипты](scripts.md)
- [Документы](../documents/README.md)
- [Контексты](../contexts/README.md)
- [Команды консоли](../console/commands.md)
