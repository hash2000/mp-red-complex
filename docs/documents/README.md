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

## 3. `DocumentWidget`

Базовый класс хранит состояние в переменных контекста:

| Переменная | Назначение |
|---|---|
| `title` | заголовок вкладки |
| `glyph` | UTF-8-иконка вкладки |
| `modified` | признак несохранённых изменений |
| переменные документа | например, `path`, `nodeId`, `selectedNodeId` |

Методы:

```cpp
QString title() const;
void setTitle(const QString& title);

QString glyph() const;
void setGlyph(const QString& glyph);

bool isModified() const;
void setModified(bool modified);

std::shared_ptr<CommandContext> context() const;
void setContext(const std::shared_ptr<CommandContext>& context);

QVariant variable(const QString& name, const QVariant& defaultValue = {});
bool setVariable(const QString& name, const QVariant& value);
bool hasVariable(const QString& name) const;

virtual QWidget* createPropertiesEditor(QWidget* parent);
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
                      "📝",
                      "Мой документ"),
    [](const QString&, const WidgetBuildHandler& handler) {
        return new MyDocument(handler.parent);
    });
```

`WidgetTypeHandler` содержит:

- `className` — полное имя класса;
- `xtype` — псевдоним для команд и описаний;
- `singletone` — один документ на приложение;
- `glyph` и `title` — значения по умолчанию.

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

## 8. Панель свойств

`DocumentWidget::createPropertiesEditor()` по умолчанию создаёт `QFormLayout`.
Наследник может добавить свои строки. `MainWindow` показывает редактор
активного документа в панели свойств.

Если документ меняет набор свойств, вызовите `emit propertiesChanged()`.

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

## 10. Правила

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
