# Скрипты SystemRouter

Скрипт — это запись `data_blobs` с полями `id`, `version`, `description`,
`data`. Узел ссылается на скрипт свойством с mime-типом
`application/x-mp-command`; значение свойства — GUID записи.

Сервис: `SystemRouterCommandService`
(`src/Content/SystemRouter/services/system_router_command_service.*`).

## 1. Как узел связывается с командой

```json
{
  "authorize": {
    "mime": ["application/x-mp-command"],
    "value": "11111111-1111-1111-1111-111111111111"
  }
}
```

- `authorize` — имя свойства; оно показывается как название скрипта;
- `value` — GUID команды (без фигурных скобок, но сервис нормализует и
  вариант с фигурными скобками);
- сама команда хранится в `data_blobs` с тем же GUID.

Свойств с командным mime-типом может быть несколько. `previewsForNode()`
возвращает по одному preview на каждое такое свойство.

## 2. Выполнение по двойному щелчку

`SystemRouterTreeDocument::onNodeActivated()`:

1. Если сервиса команд нет или у узла нет command-свойств — узел
   разворачивается/сворачивается.
2. Если скрипт один — выполняется сразу.
3. Если скриптов несколько — открывается `SystemRouterScriptDialog` со
   списком, описанием и готовым кодом; выполняется выбранный.
4. Перед выполнением создаётся дочерний контекст, в него записываются
   свойства с `application/x-mp-context-variable`.

Ошибки (команда не найдена, нет параметра, ошибка парсинга) выводятся в
консоль через `printError()` / `printWarning()`. Диалог выбора скрипта — не
диалог ошибки, а вопрос пользователю.

## 3. Подстановка `{{параметр}}`

`renderScript()` ищет блоки `{{имя}}` и заменяет их значением свойства с
mime-типом `application/x-mp-command-parameter`.

```text
echo user={{user}}, enabled={{enabled}}, body={{body}}
```

Подстановка выполняется по имени свойства среди свойств узла. Свойство должно
иметь `application/x-mp-command-parameter`; одного совпадения имени недостаточно.

Форматирование значения (`formatParameterValue`) зависит от первого обычного
mime-типа свойства (командные маркеры пропускаются):

| Mime-тип | Результат в скрипте |
|---|---|
| `application/x-boolean` | `true` / `false` |
| `application/x-integer` | целое без кавычек |
| `application/x-number` | число без кавычек |
| `text/x-multiline` | `<<text>>...<</text>>` |
| `application/json` | `<<json>>...<</json>>` |
| `application/xml`, `text/xml` | `<<xml>>...<</xml>>` |
| `text/x-ini` | `<<ini>>...<</ini>>` |
| `application/x-color` | `#AARRGGBB` в кавычках |
| остальное | строка в кавычках с экранированием `\` и `"` |

Отсутствующий параметр:

- подставляется как `<<undefined>>`;
- его имя добавляется в `missingParameters`;
- при выполнении выводится предупреждение.

## 4. Контекстные переменные

Свойства с `application/x-mp-context-variable` **не** подставляются в текст.
Они записываются в `CommandContext` перед выполнением:

```cpp
executionContext->setVariable(name, node->properties().value(name));
executionContext->setVariableMimeType(name, mimeTypes.join(", "));
```

Поэтому команда может прочитать их через:

```cpp
const auto value = context->variable("token").toString();
```

Если скрипт создаёт документ, а переменная должна быть видна этому документу,
свойству нужны оба маркера:

- `application/x-mp-context-variable` — чтобы попасть в контекст команды;
- `application/x-mp-shared` — чтобы `WidgetZoneCommand` скопировал её в
  контекст-создатель документа.

Без `application/x-mp-shared` переменная останется только в контексте
команды.

## 5. Порядок выполнения

Упрощённо:

```text
активация узла
  -> собрать command-свойства
  -> для каждого: найти запись data_blobs
  -> renderScript(script, node)
  -> создать контекст выполнения
  -> перенести context-variable свойства
  -> CommandProcessor::execute(renderedScript, executionContext)
```

Контекст выполнения — дочерний к контексту дерева/документа, поэтому команда
видит стандартные переменные приложения. Контекст команды, создаваемый
`CommandProcessor`, в свою очередь является дочерним к контексту выполнения и
имеет режим `Mutable`.

## 6. Code-editor для скрипта

Свойство-команда отображается кнопкой в панели свойств. По нажатию:

1. `ensureBlob()` берёт GUID из значения свойства;
2. если записи нет, создаёт пустую запись в `data_blobs`;
3. открывает code-editor `edit-value-blob-{guid}` с `data` и `blobId`;
4. после `document.save.complete` выполняется
   `system-router-save-blob`, и текст сохраняется в `data_blobs`.

В редакторе отображается именно `data` команды — текст скрипта. Поля
`version` и `description` при сохранении сохраняются, если запись уже
существовала.

## 7. Несколько команд

Если у узла несколько command-свойств, `previewsForNode()` возвращает
несколько `SystemRouterScriptPreview`. Диалог показывает:

- имя свойства;
- описание команды;
- подготовленный текст скрипта;
- предупреждения о недостающих параметрах.

Для невалидной команды (`commandFound == false`) кнопка запуска недоступна;
выбор такого скрипта не выполняет его.

## 8. Консольные команды для проверки

```text
system-router-blobs
system-router-routes
```

`system-router-blobs` показывает записи `data_blobs`, включая скрипты;
`system-router-routes` — узлы и их `data` (mime + `value`/`value_blob`).

## 9. Частые ошибки

**`Команда не найдена`** — значения свойства-команды нет в `data_blobs`.
Откройте свойство кнопкой: пустая запись будет создана.

**`Некорректная ссылка на команду`** — значение не является GUID. Для
команды значение должно быть GUID, а не текстом скрипта.

**Параметр не подставился** — у свойства нет
`application/x-mp-command-parameter` либо имя в `{{...}}` не совпадает с
именем свойства.

**Контекстная переменная не видна в документе** — добавьте
`application/x-mp-shared`; одного `application/x-mp-context-variable`
недостаточно.

**Значение перезаписывается** — проверьте, нет ли в JSON одновременно
`value` и `value_blob`: при чтении приоритет у `value`.

## См. также

- [Узлы](nodes.md)
- [SystemRouter](README.md)
- [Контексты](../contexts/README.md)
- [Команды консоли](../console/commands.md)
