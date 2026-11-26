# Узлы SystemRouter

Узел (`nodes`) — запись дерева маршрутизатора. У узла есть идентификатор,
родитель, имя и набор свойств. Свойства определяют, что произойдёт при
двойном щелчке и какие команды доступны узлу.

## 1. Структура узла

```sql
nodes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    parent_id INTEGER REFERENCES nodes(id) ON DELETE CASCADE,
    name TEXT NOT NULL,
    data TEXT NOT NULL DEFAULT '{}'
)
```

Свойства хранятся в `data` как JSON:

```json
{
  "authorize": {
    "mime": ["application/x-mp-command"],
    "value": "11111111-1111-1111-1111-111111111111"
  },
  "user": {
    "mime": ["application/x-mp-command-parameter", "text/plain"],
    "value_blob": "22222222-2222-2222-2222-222222222222"
  },
  "token": {
    "mime": ["application/x-mp-context-variable", "application/x-mp-shared", "text/plain"],
    "value_blob": "33333333-3333-3333-3333-333333333333"
  }
}
```

Допустимы и свойства без mime-типов:

```json
{
  "icon": {
    "mime": [],
    "value": "settings"
  }
}
```

При чтении `value_blob` подгружается из `data_blobs`; если есть и `value`, и
`value_blob`, приоритет у `value`.

## 2. Настройка через интерфейс

1. Откройте документ `system-router` (`documents action=create, target=system-router`).
2. Выберите узел и откройте «Свойства…» в контекстном меню.
3. Нажмите `+`, задайте имя свойства и набор mime-типов.
4. Для параметров и контекстных переменных задайте значение.
5. Для команды нажмите кнопку с `...` и отредактируйте скрипт в code-editor.

Свойства сохраняются сразу после изменения. Отдельной кнопки «Сохранить» нет;
ошибки сохранения выводятся в консоль.

Редактирование — отдельный документ. Активация узла с параметрами открывает
документ просмотра (только чтение), а пункт «Просмотр…» контекстного меню
делает то же самое.

Команда-свойство без значения получает сгенерированный GUID. Если позже
изменить mime-набор на командный, пустое/некорректное значение также будет
заменено GUID.

## 3. Mime-типы свойств

| Mime-тип | Роль |
|---|---|
| `text/plain` | обычный текст |
| `text/x-multiline` | многострочный текст |
| `application/json` | JSON |
| `application/xml` | XML |
| `text/x-ini` | INI |
| `application/x-boolean` | флаг |
| `application/x-integer` | целое |
| `application/x-number` | число |
| `application/x-color` | цвет |
| `application/x-mp-command` | ссылка на команду |
| `application/x-mp-command-parameter` | параметр для `{{имя}}` в скрипте |
| `application/x-mp-context-variable` | переменная для `CommandContext` |
| `application/x-mp-shared` | переменную можно передать в контекст создаваемого документа |
| `application/x-mp-secret` | секретные данные: в просмотре узла значение скрыто |
| `application/x-mp-hidden` | скрытый параметр: в просмотре узла не отображается |

Первый «обычный» mime-тип (не служебный маркер) выбирает редактор значения.
Служебные маркеры (`command`, `command-parameter`, `context-variable`, `shared`,
`secret`, `hidden`) влияют на поведение, но не на редактор.

## 4. Что попадает в `data_blobs`

`SystemRouterDataNode::propertyValueIsBlob()`:

| Свойство | `value_blob`? |
|---|---|
| есть `application/x-mp-command` | **нет**, в `value` хранится GUID |
| есть `application/x-mp-command-parameter` | да |
| есть `application/x-mp-context-variable` | да |
| есть `text/x-multiline` | да |
| есть `application/json` | да |
| есть `application/xml` | да |
| есть `text/x-ini` | да |
| остальное | нет |

`application/x-mp-shared` сам по себе не включает блок. Он лишь разрешает
копирование переменной при создании документа.

Если значение блочное, провайдер при сохранении:

1. генерирует GUID, если его ещё нет;
2. сохраняет значение в `data_blobs`;
3. записывает GUID в JSON свойства как `value_blob`.

Базовая `LazyTreeDataNode` блоков не использует; поведение включается только
в `SystemRouterDataNode`.

## 5. Команда в свойстве

Свойство-команда:

```json
"authorize": {
  "mime": ["application/x-mp-command"],
  "value": "11111111-1111-1111-1111-111111111111"
}
```

- имя свойства (`authorize`) — человекочитаемое имя скрипта в дереве и
  диалоге;
- значение — GUID записи `data_blobs`;
- сама запись содержит `data` (текст скрипта), `version`, `description`.

Несколько свойств с `application/x-mp-command` означают несколько скриптов у
узла. При активации узла (двойной щелчок или Enter):

- есть скрипты — открывается диалог выбора (`SystemRouterScriptDialog`) с
  кнопками «Запуск» и «Просмотр»; диалог показывается и для одного скрипта;
- скриптов нет, но есть параметры — открывается документ просмотра узла;
- нет ни скриптов, ни параметров — узел разворачивается/сворачивается.

Параметрами для просмотра считаются свойства узла без служебного глифа и без
mime-типа `application/x-mp-hidden`.

Подробно о подстановке параметров — в [scripts.md](scripts.md).

## 6. Параметры команды

Свойство с `application/x-mp-command-parameter` подставляется в скрипт вместо
`{{имя}}`:

```json
"user": {
  "mime": ["application/x-mp-command-parameter", "text/plain"],
  "value": "admin"
}
```

```text
echo user={{user}}
```

Значение форматируется по mime-типу: строки — в кавычках, числа — без кавычек,
boolean — `true`/`false`, многострочный текст — блоком `<<text>>`, JSON —
блоком `<<json>>`, XML — `<<xml>>`, INI — `<<ini>>`.

Если параметр не найден, в скрипт попадает `<<undefined>>`, а в консоль —
предупреждение со списком отсутствующих параметров.

## 7. Контекстные переменные

Свойство с `application/x-mp-context-variable` при выполнении скрипта
записывается в `CommandContext` как переменная:

```json
"token": {
  "mime": ["application/x-mp-context-variable", "text/plain"],
  "value": "secret"
}
```

В скрипте `{{token}}` **не** подставляется: переменная просто доступна команде
через `context->variable("token")`.

Если команда создаёт документ, а свойству нужен переход в контекст этого
документа, добавьте также `application/x-mp-shared`:

```json
"token": {
  "mime": [
    "application/x-mp-context-variable",
    "application/x-mp-shared",
    "text/plain"
  ],
  "value": "secret"
}
```

Тогда `WidgetZoneCommand` скопирует переменную в контекст-создатель, и новый
документ увидит её через цепочку контекстов.

## 8. Открытие code-editor

Кнопка в панели свойств появляется, если в наборе mime-типов есть хотя бы один
из маркеров:

- `application/x-mp-command`;
- `application/x-mp-command-parameter`;
- `application/x-mp-context-variable`.

По нажатию:

1. для команды используется GUID из значения свойства; если записи нет,
   создаётся пустая;
2. для параметра/контекстной переменной создаётся блок и GUID записывается в
   `value_blob`;
3. открывается `code-editor` с id `edit-value-blob-{guid}`;
4. в контекст редактора передаются `data` и `blobId`;
5. по событию `document.save.complete` команда `system-router-save-blob`
   сохраняет данные в `data_blobs`.

Один блок — один редактор: повторное открытие активирует существующий
документ.

## 9. Редактор значения и блоки

Для обычных свойств редактор выбирается по первому mime-типу:

- `text/plain` — строка;
- `text/x-multiline`, JSON, XML — многострочный редактор;
- integer/number/boolean/color — соответствующий редактор;
- `text/x-file-path`, `text/x-directory-path` — выбор пути.

Для блочных свойств значение при чтении подгружается из `data_blobs`, поэтому
UI показывает текст, число или структуру, а не GUID.

## 10. Просмотр узла

Документ `system-router-node-view` (`SystemRouterNodeViewDocument` с виджетом
`SystemRouterNodeViewWidget`) показывает параметры узла только для чтения.

- Открывается при активации узла с параметрами, кнопкой «Просмотр» в диалоге
  скриптов и пунктом «Просмотр…» контекстного меню дерева; id документа —
  `system-router-node-view-{id узла}`.
- Заголовок: глиф узла (h3), название узла (h3) и идентификатор (h4) —
  отдельными метками `SystemRouterNodeViewGlyph`, `...NodeName`, `...NodeId`.
- Параметры отсортированы по имени и выровнены по левому верхнему краю;
  при большом количестве параметров появляется вертикальная прокрутка.
- Каждый параметр — блок: название (h3) и рядом маленькие значки копирования
  имени и значения, под названием — виджет значения (h4).
- Вид значения выбирается по mime-типу: строка, число, флаг, цвет,
  изображение `image/*` (путь, data-URL или base64), многострочный текст,
  JSON (с форматированием), XML/INI, ссылка на команду. Для параметра-команды
  показывается текст скрипта из `data_blobs`, а не GUID блока; копируется тоже
  текст скрипта.
- `application/x-mp-secret` — параметр виден, значение скрыто и не копируется.
- `application/x-mp-hidden` — параметр в просмотре не отображается.
- Служебное свойство глифа (`glyph`/`icon`) не показывается.
- Идентификатор узла хранится в переменной контекста `nodeId`, поэтому
  документ обновляется при изменении узла в «Свойствах…».

Вид оформления задаётся темой в `assets/themes/{тема}/stylesheets/system-router.qss`
(файл подключён в `stylesheets/index.yaml`): размеры h3/h4, цвета глифа,
названия и идентификатора, значки копирования, виджетов значений. В коде
виджета задаются только `objectName`; `setStyleSheet()` не используется.

## 11. Ограничения и проверки

- имя узла не пустое;
- имя уникально среди соседей без учёта регистра;
- нельзя переместить узел внутрь собственного поддерева;
- удаление узла каскадно удаляет потомков на уровне БД;
- при ошибке сохранения изменение откатывается;
- перемещение сохраняется в БД до перестроения модели.

## 12. Пример целиком

```json
{
  "authorize": {
    "mime": ["application/x-mp-command"],
    "value": "11111111-1111-1111-1111-111111111111"
  },
  "user": {
    "mime": ["application/x-mp-command-parameter", "text/plain"],
    "value": "admin"
  },
  "server": {
    "mime": ["application/x-mp-context-variable", "application/x-mp-shared", "text/plain"],
    "value": "prod"
  }
}
```

Скрипт команды:

```text
echo user={{user}}, server={{server}}
```

`{{user}}` будет заменён, `{{server}}` останется как есть, а `server` попадёт
в контекст команды и в контекст созданного ею документа.

## См. также

- [Скрипты](scripts.md)
- [SystemRouter](README.md)
- [Контексты](../contexts/README.md)
- [Документы](../documents/README.md)
