# Документы

Документ — это любой виджет, созданный через `MainWindow::createWidget()` в
зоне `Document`, `BottomPanel` или `SidePanel`. Даже если фабрика вернула
обычный `QWidget`, `MainWindow` обернёт его в `DocumentWidgetStub`, поэтому у
такого виджета всё равно будет контекст документа.

Основные классы:

- `MainWindow` — `src/Libs/Engine/ui/main_window.h`;
- `DocumentWidget` — `src/Libs/Engine/ui/doc_widget.h`;
- `WidgetRegistry` — `src/Libs/Engine/scripting/widgets/widget_registry.h`;
- `WidgetZoneCommand` — `src/Libs/Engine/commands/widget_zone_command.cpp`.

## 1. Создание из консоли

Документы и панели создаются командами зон:

```text
documents action=create, target=system-router, id=router
info-panels action=create, target=console, id=console
side-panels action=create, target=system-router-node-properties, id=properties
```

Параметры:

| Параметр | Значение |
|---|---|
| `action` | `create`, `show`, `close`, `list` |
| `target` | xtype или полное имя класса из `WidgetRegistry` |
| `id` | идентификатор документа; без него генерируется UUID |
| `name`, `title`, `glyph`, `tooltip` | параметры создаваемого виджета |
| остальные параметры | становятся начальными переменными контекста документа |

Особенности:

- повторный `id` в той же зоне активирует существующий виджет;
- `id`, занятый виджетом в другой зоне, отклоняется;
- для типа-синглтона `id` всегда равен его `xtype`;
- параметры создаваемого виджета записываются в контекст **до** вызова
  `onContextAttached()`, поэтому документ может прочитать их при инициализации;
- переменные с mime-типом `application/x-mp-shared`, видимые в контексте
  команды, копируются в контекст-создатель документа.

## 2. Создание из C++

```cpp
auto* window = controller->mainWindow();
if (!window) {
    return;
}

const QVariantMap variables = {
    { "path", "C:/data/notes.txt" },
    { "nodeId", 42 }
};

auto* document = window->createWidget(
    "notes",                                  // id
    MainWindow::WidgetKind::Document,         // зона
    "Заметки",                                // title
    "📝",                                     // glyph
    [] { return new MyDocumentWidget(); },    // фабрика
    "Редактор заметок",                       // tooltip
    "my-document",                            // xtype/class
    context,                                  // creatorContext
    variables);                               // начальные переменные
```

`creatorContext` определяет положение контекста документа:

- документ из консоли или загрузчика описания приложения получает
  `.documents.{id}` (создатель — `.documents`);
- документ, созданный другим документом, получает
  `.documents.{creator}.documents.{id}`.

`WidgetZoneCommand` выбирает создателя так: ближайший документ-контекст
команды (`owningDocumentContext()`), иначе глобальный контекст. Поэтому выбор
контекста в консоли не меняет место нового документа.

Фабрика возвращает `QWidget*`. `WidgetRegistry::build()` оборачивает результат
в `DocumentWidget`, если это ещё не документ-обёртка. Владельцем созданного
виджета становится `MainWindow`.

Диалог создания документа (`NewDocumentDialog`, Ctrl+N) задаёт параметры
нового документа: имя (`id`), значок (по умолчанию — из темы), заголовок
и **цвет вкладки**. Цвет передаётся в `variables()` переменной `color`
(`QVariant<QColor>`); пустое значение переменную не создаёт, и документ
получает цвет из темы.

```cpp
NewDocumentDialog dialog(registry, context);
if (dialog.exec() == QDialog::Accepted) {
    const auto variables = dialog.variables();   // включает color, если выбран
    const auto color = dialog.documentColor();   // QColor(), если не выбран
}
```

## 3. `DocumentWidget`

Базовый класс хранит состояние в переменных контекста:

| Переменная | Назначение |
|---|---|
| `title` | заголовок вкладки |
| `modified` | признак несохранённых изменений |
| `glyph` | явный UTF-8-значок вкладки; пустой — берётся из темы |
| `color` | явный цвет вкладки (`QColor`); невалидный — берётся из темы |
| `xtype` | тип документа, которым он создан (используется группировкой) |
| `className` | полное имя класса документа (по нему тема задаёт оформление вкладки) |
| переменные документа | например, `path`, `nodeId`, `selectedNodeId` |

Методы:

```cpp
QString title() const;
void setTitle(const QString& title);

QString glyph() const;              // действующий значок: явный → тема → «📄»
void setGlyph(const QString& glyph); // пустой сбрасывает к значку темы
QString themeGlyph() const;          // значок из темы для класса/xtype
void setThemeGlyph(const QString& glyph);
static const QString& defaultGlyph();

bool isModified() const;
void setModified(bool modified);

QColor color() const;               // цвет вкладки
void setColor(const QColor& color); // невалидный цвет сбрасывает в цвет темы

QString xtype() const;
void setXtype(const QString& xtype);

QString className() const;
void setClassName(const QString& className);

std::shared_ptr<CommandContext> context() const;
void setContext(const std::shared_ptr<CommandContext>& context);

QVariant variable(const QString& name, const QVariant& defaultValue = {});
bool setVariable(const QString& name, const QVariant& value);
bool hasVariable(const QString& name) const;

virtual QWidget* createPropertiesEditor(QWidget* parent);
virtual QList<DocumentPropertyGroup> createPropertyGroups(QWidget* parent);
virtual QWidget* createTabPropertiesEditor(QWidget* parent);
bool showProperties(const QString& groupId = {});
virtual void refresh();
virtual bool save();
virtual bool maybeClose();
```

Жизненный цикл:

1. `MainWindow::createWidget()` создаёт документ и контекст.
2. Начальные переменные записываются в контекст.
3. Документу назначается контекст; вызывается `onContextAttached()`.
4. Изменения переменных приходят в `onContextVariableChanged()`.
5. `refresh()` вызывается кнопкой обновления в панели свойств.
6. `save()` вызывается при закрытии изменённого документа; `maybeClose()`
   решает, можно ли закрыть документ.
7. Изменения шапки документа порождают `titleChanged`, `glyphChanged`,
   `modifiedChanged`; изменение внутренних свойств — `propertiesChanged()`.

### `onContextAttached()`

```cpp
void MyDocument::onContextAttached() {
    const auto path = variable("path", QString()).toString();
    if (!path.isEmpty()) {
        loadFile(path);
    }
}
```

Если переменной нет, `variable()` у `DocumentWidget` создаст её со значением
по умолчанию. Не вызывайте `setContext()` вручную для документов, созданных
через `MainWindow`: это делает окно.

### `onContextVariableChanged()`

```cpp
void MyDocument::onContextVariableChanged(const QString& name, const QVariant& value) {
    if (name == "path") {
        loadFile(value.toString());
    }
}
```

Не забывайте про защиту от рекурсии, если сами пишете ту же переменную из
обработчика.

## 4. Зоны и идентификаторы

`MainWindow::WidgetKind`:

- `Document` — вкладки в центральной области;
- `BottomPanel` — нижняя панель;
- `SidePanel` — боковая панель.

Полезные методы:

```cpp
QWidget* widget(const QString& id) const;
DocumentWidget* document(const QString& id) const;
std::shared_ptr<CommandContext> documentContext(const QString& id) const;
QString activeDocumentId() const;
QStringList widgetIds() const;
QStringList widgetIds(WidgetKind kind) const;
bool showWidget(const QString& id);
bool closeWidget(const QString& id);
```

ID документа должен быть стабильным: по нему ищется существующий виджет и
формируется контекст. Для обычных документов можно передать пустой `id` —
`WidgetZoneCommand` сгенерирует UUID.

### Синглтоны

Если тип зарегистрирован с `singletone = true`, `WidgetRegistry::documentId()`
возвращает `xtype` независимо от запрошенного id. Повторное создание
активирует уже открытый экземпляр. Так обычно работают панели свойств и другие
единственные в приложении документы.

## 5. Регистрация типа

В `src/Launcher/widgets_builder.cpp`:

```cpp
registry->registerFactory(
    WidgetTypeHandler("Content.My.Module.MyDocument",
                      "my-document",
                      false,
                      "Мой документ"),
    [](const QString&, const WidgetBuildHandler& handler) {
        return new MyDocument(handler.parent);
    });
```

`WidgetTypeHandler` содержит:

- `className` — полное имя класса;
- `xtype` — псевдоним для команд и описаний;
- `singletone` — один документ на приложение;
- `title` — подпись по умолчанию;
- `rule` — роль документа (`DocumentRule`), см. раздел 11.

Оформление вкладки по умолчанию (стандартный глиф и цвет) в регистрации
**не задаётся** — его описывает тема в `assets/themes/{theme}/documents.yaml`
по классу документа или `xtype` (см. раздел 10). Явные глиф и цвет документа
задаются при создании (`glyph=` в команде, `glyph:` в yaml, панель свойств).

`WidgetBuildHandler` дополнительно содержит `name`, `parent`, `options`,
`context`.

## 6. Метаданные документа

`IDocumentMetadata` (`src/Libs/Engine/scripting/widgets/i_document_metadata.h`)
описывает обязательные переменные типа. Метаданные регистрируются по полному
имени класса:

```cpp
registry->registerDocumentMetadata(
    "Content.My.Module.MyDocument",
    std::make_unique<MyDocumentMetadata>());
```

`MyDocumentMetadata::variables()` возвращает список `DocumentVariable`:
имя переменной и mime-тип. Диалог создания документа использует эти данные,
чтобы показать поля и выбрать редактор значения.

## 7. Обмен состоянием

Документы не должны напрямую вызывать методы друг друга. Используйте контекст:

```cpp
// В документе A
setVariable("selectedId", id);

// В документе B
connect(context().get(), &CommandContext::variableChanged, this,
    [this](const QString&, const QString& name, const QVariant&, const QVariant& value) {
        if (name == "selectedId") {
            showDetails(value);
        }
    });
```

Если нужно уведомить о завершении операции:

```cpp
context()->fireEvent(CommandContext::documentSaveCompleteEvent());
```

Для пакетного обновления используйте `beginVariablesBatch()` /
`endVariablesBatch()` — подписчики получат одно событие
`variables.batch.complete`.

## 8. Панель свойств и лента документа

Принцип: **функции — в ленте документа, параметры — в панели «Свойства»**.
Любой параметр (в том числе параметры функциональной кнопки) редактируется в
панели свойств; в ленте остаются только действия.

Панель показывает параметры активного документа сгруппированными разделами
(`PropertiesGroupWidget`,
`src/Libs/BaseWidgets/properties/properties_group_widget.*`):

| Группа (`id`) | Содержимое |
|---|---|
| `document` — «Свойства документа» | `createPropertiesEditor()` и вложенные группы `createPropertyGroups()` (например, `search` — «Параметры поиска») |
| `tab` — «Свойства вкладки» | `createTabPropertiesEditor()`: заголовок, значок, цвет (`TabColorEditor`), тип, признак изменений |
| `context` — «Параметры контекста» | имя контекста (только для чтения, кнопка копирования) и вложенная группа `context.variables` |
| `context.variables` — «Переменные контекста» | список видимых переменных; **строится лениво** — при первом раскрытии |

API панели: `setDocument`, `revealGroup(id)`, `isGroupExpanded(id)`,
`groupIds()`, сигнал `groupExpanded(id)`. Состояние раскрытия групп
сохраняется между перестройками панели.

### Документ и группы свойств

```cpp
QWidget* createPropertiesEditor(QWidget* parent);                    // «Свойства документа»
QList<DocumentPropertyGroup> createPropertyGroups(QWidget* parent);  // вложенные группы
QWidget* createTabPropertiesEditor(QWidget* parent);                 // «Свойства вкладки»
```

`DocumentPropertyGroup` содержит `id`, `title`, `editor` и `expanded`:

```cpp
QList<DocumentPropertyGroup> MyDocument::createPropertyGroups(QWidget* parent) {
    DocumentPropertyGroup group;
    group.id = "filters";
    group.title = tr("Фильтры");
    group.editor = buildFiltersEditor(parent);
    return { group };
}
```

`createPropertiesEditor()` по умолчанию возвращает пустой `QFormLayout`;
наследник добавляет строки. Если у документа нет своих параметров и
дополнительных групп, группа «Свойства документа» не создаётся.

### Значок свойств у кнопки ленты

`DocumentRibbon` и `RibbonButton` (`src/Libs/BaseWidgets/ribbon/`) — общая
лента документа. Кнопка с параметрами создаётся с идентификатором группы:

```cpp
auto* group = ribbon->addGroup(tr("Поиск"));
auto* search = ribbon->addButton(group, "🔍", tr("Поиск"), "search");
connect(search, &RibbonButton::clicked, this, ...);           // действие
connect(ribbon, &DocumentRibbon::propertiesRequested, this,  // значок ⚙
        [this](const QString& id) { showProperties(id); });
```

Большая часть кнопки выполняет действие, маленький круглый значок в правом
нижнем углу раскрывает группу свойств действия: `DocumentWidget::showProperties(id)`
находит окно-оболочку и вызывает `MainWindow::showDocumentProperties(doc, id)`,
которая показывает панель «Свойства», раскрывает группу и её предков.

Если документ меняет набор свойств, вызовите `emit propertiesChanged()`.

Списки свойств внутри групп (`PropertiesListWidget`) работают без собственного
вертикального скролла (`setScrollable(false)`): они растягиваются по высоте,
а прокручивается вся панель свойств. Собственный скролл остаётся у списков,
которые используются вне панели (диалог создания документа и т. п.).

### Редактор кода

В ленте редактора остаются функции «Файл», «Правка» (форматирование) и
«Поиск». Параметры поиска (`search`) — регулярное выражение, упрощённое
выражение, регистр и слово целиком — редактируются в панели свойств; значок
свойств у кнопки поиска раскрывает их. Язык подсветки и размер шрифта —
параметры документа в группе «Свойства документа».

### Переменные контекста

В группе «Переменные контекста» показаны все видимые переменные контекста
(собственные и унаследованные), их значения только для чтения и подсказка:
собственная или унаследованная переменная и mime-тип значения. Список
обновляется по сигналу `CommandContext::variableChanged`, но создаётся только
после раскрытия группы. У документа без контекста группа не создаётся.

## 9. Code editor: `path` и `data`

`CodeEditorDocument` поддерживает два режима:

- обычный файл: путь в `path`, сохранение пишет в файл;
- данные: содержимое в `data`, редактор не требует файла.

Правила режима `data`:

1. Если есть `path` и файл существует — источник истины файл; `data` не
   изменяется, пользователю выводится предупреждение.
2. Если `path` задан, но файла нет — редактор открывает `data`, а при
   сохранении обновляет и `data`, и файл.
3. Если `path` нет — сохранение обновляет только `data`, диалог выбора файла
   не показывается.
4. После успешного сохранения отправляется `document.save.complete`.

Используйте этот режим для значений SystemRouter: свойства с блоком
открываются в code-editor, а сохранение возвращает текст в `data_blobs`.

## 10. Вкладки: цвет, порядок и группировка

Панель открытых документов (`DocumentTabsPanel`,
`src/Libs/Engine/ui/doc_tab_panel.*`) показывает вкладки так, как решило
**правило сортировки/группировки**. Правила лежат в
`src/Libs/Engine/ui/doc_tab_panel/document_tabs_order.*`:

| Правило (`id`) | Что делает | Ручная сортировка |
|---|---|---|
| `manual` | порядок задаёт пользователь | да |
| `color` | группы по цвету вкладки (`color`), «Без цвета» — последняя | нет |
| `xtype` | группы по типу документа (`xtype`), «Без типа» — последняя | нет |

Активное правило хранится в системном параметре `documents.tabs.order`
(переменная глобального контекста) и меняется в документе системных настроек
(кнопка ⚙ на панели кнопок справа). Одновременно действует одно правило.

Оформление вкладки (цвет и стандартный глиф) задаётся двумя способами, они
складываются по приоритету:

1. **свои значения документа** — переменные `color` и `glyph`; цвет и глиф
   редактируются в панели свойств («Цвет вкладки», «Значок»). Пустое значение
   означает «взять из темы»;
2. **значения из темы** — привязка к классу документа или его `xtype` в файле
   `assets/themes/{theme}/documents.yaml`.

```yaml
# assets/themes/{theme}/documents.yaml
tabs:
  default: "#3c3c3c"            # необязательный цвет для всех вкладок
  defaultGlyph: "📄"             # необязательный глиф для всех вкладок
  colors:
    Content.CodeEditor.CodeEditorDocument: "#61afef"  # полное имя класса
    code-editor: "#61afef"                            # xtype
  glyphs:
    Content.CodeEditor.CodeEditorDocument: "📝"
    code-editor: "📝"
```

Ключ — полное имя класса (`WidgetTypeHandler::className`) или `xtype`; класс
важнее `xtype`, `default`/`defaultGlyph` используются, если привязки нет.
Стандартные глифы раньше задавались при регистрации типа в `WidgetsBuilder`
(`WidgetRegistry::registerFactory`) — теперь они живут в теме.

Панель вкладок читает файл из каталога темы (`path.theme` в ресурсах) при
создании: цвета применяются к уже открытым документам и участвуют
в группировке по цвету, а глиф передаётся документу как запасной
(`DocumentWidget::setThemeGlyph`). Если глифа нет ни у документа, ни в теме,
действует `DocumentWidget::defaultGlyph()` (`📄`). Панель рисует цветную
полосу слева от значка.

```cpp
// Оформление можно задать и программно (например, в тестах):
panel->setDocumentTabTheme(DocumentTabTheme::loadFromThemeDirectory(themeDir));
```

Перетаскивание вкладок мышью работает при правиле `manual`: нажатие на
вкладку и движение вверх/вниз показывает линию вставки, отпускание меняет
порядок. При группировке перетаскивание отключено — порядок задаёт правило.

Чтобы добавить новый вариант, реализуйте `IDocumentTabsOrderStrategy` и
зарегистрируйте его в конструкторе `DocumentTabsOrderRegistry`; системный
параметр и интерфейс настроек соберут список значений сами
(`registerDocumentTabsOrderOption`).

```cpp
// Программная сортировка (например, из команды или теста)
panel->moveDocument(document, 0);      // на первую позицию
panel->setOrderStrategy("xtype");      // включить группировку по типу
```

## 11. Роль документа: `DocumentRule`

`WidgetTypeHandler::rule` описывает роль документа. Это не отдельный вид
виджета: документ сохраняет все обычные функции, но получает дополнительные
привилегии и ограничения. Правила роли применяет **interceptor контекста**
(`IContextInterceptor`, `src/Libs/Engine/scripting/command_context/*`): он
решает, в каком контексте создаётся документ, какой у контекста
`VariablesMode`, разрешены ли события и изменение переменных. Проверки правил
приоритетнее режима переменных.

| Роль | Контекст | `VariablesMode` | Назначение |
|---|---|---|---|
| `DocumentRule::Document` | `.documents.{id}` | `Lazy` | обычный документ (текущее поведение) |
| `DocumentRule::SystemOptions` | `.options` | `Mutable` | документ системных настроек |

```cpp
registry->registerFactory({ SystemOptionsDocument::className(),
                            SystemOptionsDocument::xtype(),
                            /*singletone*/ true,
                            QStringLiteral("⚙"),
                            QStringLiteral("Системные настройки"),
                            DocumentRule::SystemOptions },
    [](const QString&, const WidgetBuildHandler& handler) {
        return new SystemOptionsDocument(handler.parent);
    });
```

`MainWindow::createWidget()` берёт роль из `WidgetRegistry::typeInfo(type)` и
передаёт её в `CommandContext::createDocumentContext(id, rule)`. Правила
создаются фабрикой `createContextInterceptor(rule)`, поэтому новая роль — это
новый interceptor и одна ветка фабрики.

## 12. Системные настройки

Системные параметры приложения — переменные **глобального контекста**. Их
описания (подпись, тип, допустимые значения, значение по умолчанию) хранит
`SystemOptionsRegistry` (`src/Libs/Engine/scripting/system_options/`).

Правила доступа:

- `.documents` создаётся как `ReadOnly`, поэтому документы не могут менять
  системные параметры: их записи остаются в собственных переменных;
- документ с ролью `DocumentRule::SystemOptions` работает в контексте
  `.options` в режиме `Mutable`: его записи попадают в глобальный контекст;
- значения по умолчанию записывает `CommandController::initSystemContexts()`
  при запуске, поэтому настройки существуют независимо от открытого документа.

Документ настроек — `SystemOptionsDocument`
(`src/Libs/Engine/ui/main_window/system_options_document.*`), xtype
`system-options`. Он открывается кнопкой ⚙ на панели кнопок справа или
командой:

```text
documents action=create, target=system-options
```

Новый системный параметр добавляется регистрацией описания и (при
необходимости) значения по умолчанию:

```cpp
SystemOption option;
option.name = "documents.tabs.order";
option.title = "Сортировка вкладок документов";
option.type = SystemOptionType::Enum;
option.values = { "manual", "color", "xtype" };
option.valueTitles = { "Ручная сортировка", "Группировка по цвету", "Группировка по типу" };
option.defaultValue = "manual";
controller->systemOptions()->registerOption(option);
```

## 13. Правила

- Не создавайте документы в обход `MainWindow::createWidget()`.
- Не назначайте `DocumentWidget::setContext()` вручную, если документ создан
  окном.
- Не храните путь и состояние в статических переменных — используйте контекст.
- Для ошибок используйте `printError()` / `printWarning()`, а не `QMessageBox`.
- При создании дочернего документа передавайте его контекст в
  `createWidget(..., creatorContext, ...)`, чтобы сохранить цепочку владения.
- Для одноразовых вычислений создавайте дочерний контекст, а не меняйте
  глобальные переменные.

## См. также

- [Контексты](../contexts/README.md)
- [Команды консоли](../console/commands.md)
- [SystemRouter](../system-router/README.md)
- [Резервное копирование БД](../database-backup/README.md)
