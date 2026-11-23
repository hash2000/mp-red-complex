#include "Content/CodeEditorWidget/documents/code_editor_document.h"
#include "Content/CodeEditorWidget/documents/code_editor_document_metadata.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/widgets/code_editor_search_panel.h"
#include "Content/CodeEditorWidget/widgets/code_editor_widget.h"
#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/BaseWidgets/ribbon/document_ribbon.h"
#include "Libs/BaseWidgets/ribbon/ribbon_button.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/main_window/new_document_dialog.h"
#include "Libs/Engine/ui/properties_panel.h"
#include "Libs/Resources/resources.h"

#include <QCheckBox>
#include <QComboBox>
#include <QCompleter>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStringConverter>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTextLayout>
#include <QToolButton>
#include <QtTest/QtTest>

#include <memory>

namespace {
class TestCommandController : public CommandController {
public:
	TestCommandController() : CommandController(nullptr, nullptr) {}
	~TestCommandController() override = default;

	CommandProcessor* commandProcessor() const override {
		return nullptr;
	}
	ServicesRegistry* services() const override {
		return nullptr;
	}

private:
	void initContext() override {
	}
};

/// Кнопка ленты, раскрывающая указанную группу свойств.
RibbonButton* findRibbonButton(QWidget* root, const QString& propertiesGroup) {
	for (auto* button : root->findChildren<RibbonButton*>()) {
		if (button->propertiesGroup() == propertiesGroup) {
			return button;
		}
	}

	return nullptr;
}

/// Значение Boolean-свойства в PropertiesListWidget.
bool propertyValue(QWidget* root, const QString& id) {
	for (auto* child : root->findChildren<QWidget*>()) {
		if (child->property("propertyId").toString() != id) {
			continue;
		}

		auto* checkBox = qobject_cast<QCheckBox*>(child);
		if (!checkBox) {
			checkBox = child->findChild<QCheckBox*>();
		}

		return checkBox && checkBox->isChecked();
	}

	return false;
}

/// Каталог эталонной темы: из него загружаются правила подсветки.
QString themeDirectory() {
	return QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR) + QStringLiteral("/assets/themes/dark");
}

/// Сколько строк документа получили форматы подсветки.
int formattedBlocks(QTextDocument* document) {
	int count = 0;
	for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
		if (block.layout() && !block.layout()->formats().isEmpty()) {
			count++;
		}
	}
	return count;
}

/// Редактор группы свойств документа по её id.
QWidget* groupEditor(const QList<DocumentPropertyGroup>& groups, const QString& id) {
	for (const auto& group : groups) {
		if (group.id == id) {
			return group.editor;
		}
	}
	return nullptr;
}
} // namespace

/// Форматтер для тестов: отвечает синхронно, без внешнего процесса.
class TestFormatter : public FormatterPlugin {
	Q_OBJECT
public:
	enum class Mode {
		Success,
		Failure,
	};

	explicit TestFormatter(Mode mode) : _mode(mode) {}

	QString mimeType() const override {
		return QStringLiteral("text/markdown");
	}

	void formatAsync(const QString& text) override {
		if (_mode == Mode::Failure) {
			emit formattingFailed(QStringLiteral("test formatter failure"));
			return;
		}

		QString formatted = text;
		formatted.replace(QStringLiteral("bad"), QStringLiteral("good"));
		emit formattingFinished(formatted);
	}

	void stop() override {}

private:
	Mode _mode;
};

class TestCodeEditorSearch : public QObject {
	Q_OBJECT

private slots:
	void init();
	void cleanup();

	void plainSearchIsCaseInsensitiveByDefault();
	void caseSensitiveOption();
	void wholeWordOption();
	void regularExpressionOption();
	void simplifiedExpressionMatchesNewline();
	void invalidRegexKeepsNoMatches();
	void nextAndPreviousWrapAround();
	void clearRemovesHighlights();
	void longLineScrollsToMatchStart();
	void documentPathComesFromContext();
	void documentPathFromCreationVariables();
	void dataVariablePopulatesEditor();
	void saveDataUpdatesContextVariable();
	void dataAndExistingFilePrefersFile();
	void dataAndMissingFileSavesBoth();
	void documentMetadataDeclaresPath();
	void newDocumentDialogShowsCodeEditorPath();
	void searchPropertiesGroupIsExposed();
	void documentInfoGroupShowsCounts();
	void encodingComboIsSearchableAndApplied();
	void searchPropertiesToggleModes();
	void ribbonSearchButtonControlsPanel();
	void ribbonSearchButtonOpensProperties();
	void propertiesVisibleOnFirstShow();
	void smallDocumentHighlightsAllBlocks();
	void largeDocumentHighlightsOnlyVisibleBlocks();
	void longLineDisablesHighlighting();
	void longLineSuppressionIsReportedToConsole();
	void manualHighlightingOverridesLongLines();
	void manualHighlightingIsReportedToConsole();
	void ribbonHighlightButtonToggles();
	void newTextResetsHighlightingPolicy();
	void longLinesEnableAutoWrap();
	void lineWrapComboControlsWrap();
	void formatAppliesResultAndKeepsCursor();
	void formatFailureIsReported();

private:
	QPlainTextEdit* _editor = nullptr;
	CodeEditorSearchPanel* _panel = nullptr;
};

void TestCodeEditorSearch::init() {
	_editor = new QPlainTextEdit;
	_editor->setPlainText(QStringLiteral("foo bar foo Foo\nsecond foo line"));
	_panel = new CodeEditorSearchPanel(_editor);
}

void TestCodeEditorSearch::cleanup() {
	delete _panel;
	_panel = nullptr;
	delete _editor;
	_editor = nullptr;
}

void TestCodeEditorSearch::plainSearchIsCaseInsensitiveByDefault() {
	_panel->setQuery(QStringLiteral("foo"));
	// "foo", "foo", "Foo" и "foo" во второй строке.
	QCOMPARE(_panel->matchCount(), 4);
	QCOMPARE(_panel->highlightedCount(), 4);
}

void TestCodeEditorSearch::caseSensitiveOption() {
	_panel->setOption(CodeEditorSearchPanel::Option::CaseSensitive, true);
	_panel->setQuery(QStringLiteral("foo"));
	QCOMPARE(_panel->matchCount(), 3);

	_panel->setOption(CodeEditorSearchPanel::Option::CaseSensitive, false);
	_panel->setQuery(QStringLiteral("foo"));
	QCOMPARE(_panel->matchCount(), 4);
}

void TestCodeEditorSearch::wholeWordOption() {
	// "foobar" не должно совпадать со "foo" при поиске по слову целиком.
	_editor->setPlainText(QStringLiteral("foo foobar foo"));
	_panel->setOption(CodeEditorSearchPanel::Option::WholeWord, true);
	_panel->setQuery(QStringLiteral("foo"));
	QCOMPARE(_panel->matchCount(), 2);

	_panel->setOption(CodeEditorSearchPanel::Option::WholeWord, false);
	_panel->setQuery(QStringLiteral("foo"));
	QCOMPARE(_panel->matchCount(), 3);
}

void TestCodeEditorSearch::regularExpressionOption() {
	_editor->setPlainText(QStringLiteral("id=10 id=20 id=abc"));
	_panel->setOption(CodeEditorSearchPanel::Option::RegularExpression, true);
	_panel->setQuery(QStringLiteral("id=\\d+"));
	QCOMPARE(_panel->matchCount(), 2);
}

void TestCodeEditorSearch::simplifiedExpressionMatchesNewline() {
	_editor->setPlainText(QStringLiteral("alpha\nbeta"));

	_panel->setOption(CodeEditorSearchPanel::Option::SimplifiedExpression, true);
	_panel->setQuery(QStringLiteral("alpha\\nbeta"));
	QCOMPARE(_panel->matchCount(), 1);

	// В обычном режиме последовательность ищется буквально и не находится.
	_panel->setOption(CodeEditorSearchPanel::Option::SimplifiedExpression, false);
	_panel->setQuery(QStringLiteral("alpha\\nbeta"));
	QCOMPARE(_panel->matchCount(), 0);
}

void TestCodeEditorSearch::invalidRegexKeepsNoMatches() {
	_panel->setOption(CodeEditorSearchPanel::Option::RegularExpression, true);
	_panel->setQuery(QStringLiteral("[unclosed"));
	QCOMPARE(_panel->matchCount(), 0);
	QCOMPARE(_panel->highlightedCount(), 0);
}

void TestCodeEditorSearch::nextAndPreviousWrapAround() {
	_editor->setPlainText(QStringLiteral("x x x"));
	_panel->setQuery(QStringLiteral("x"));
	QCOMPARE(_panel->matchCount(), 3);

	// Переходы не выходят за пределы списка совпадений.
	_panel->activateNext();
	_panel->activateNext();
	_panel->activateNext();
	_panel->activateNext();
	_panel->activatePrevious();
	_panel->activatePrevious();
	QCOMPARE(_panel->matchCount(), 3);
}

void TestCodeEditorSearch::clearRemovesHighlights() {
	_panel->setQuery(QStringLiteral("foo"));
	QVERIFY(_panel->highlightedCount() > 0);

	_panel->clearSearch();
	QCOMPARE(_panel->highlightedCount(), 0);
	QCOMPARE(_panel->matchCount(), 0);
}

void TestCodeEditorSearch::longLineScrollsToMatchStart() {
	// Длинная строка без переносов: совпадение далеко от начала.
	const QString filler(4000, QLatin1Char('x'));
	_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
	_editor->setPlainText(filler + QStringLiteral("needle") + filler);
	_editor->resize(320, 200);
	_editor->show();
	QCoreApplication::processEvents();

	_panel->setQuery(QStringLiteral("needle"));
	QCOMPARE(_panel->matchCount(), 1);

	// Видимая область заранее уведена далеко вправо.
	auto* horizontal = _editor->horizontalScrollBar();
	horizontal->setValue(horizontal->maximum());
	QVERIFY(horizontal->value() > 0);

	_panel->activateNext();
	QCoreApplication::processEvents();

	// Начало совпадения должно попасть в видимую область, а не только его хвост.
	QTextCursor start(_editor->document());
	start.setPosition(_editor->textCursor().selectionStart());
	const int left = _editor->cursorRect(start).left();
	QVERIFY2(left >= -2, qPrintable(QStringLiteral("match start x = %1").arg(left)));
}

void TestCodeEditorSearch::documentPathComesFromContext() {
	// Каталог объявлен раньше документа: он уничтожается после него,
	// когда поток чтения файла уже остановлен.
	QTemporaryDir dir;
	QVERIFY(dir.isValid());

	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	TestCommandController controller;

	CodeEditorDocument document(&highlighting, &formatters);
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("editor"));
	QVERIFY(context);

	// При привязке контекста документ создаёт path со значением по умолчанию.
	document.setContext(context);
	QVERIFY(context->ownVariables().contains(QStringLiteral("path")));
	QCOMPARE(context->ownVariables().value(QStringLiteral("path")).toString(), QString());
	QCOMPARE(document.filePath(), QString());

	const auto path = dir.filePath(QStringLiteral("sample.md"));
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
	file.write("# sample");
	file.close();

	// Изменение переменной контекста открывает файл в документе.
	context->setVariable(QStringLiteral("path"), path);
	QCOMPARE(document.filePath(), path);
	QCOMPARE(document.title(), QStringLiteral("sample.md"));
	QCOMPARE(document.variable(QStringLiteral("path")).toString(), path);

	// «Новый файл» очищает path в контексте.
	document.newFile();
	QCOMPARE(context->ownVariables().value(QStringLiteral("path")).toString(), QString());
	QCOMPARE(document.filePath(), QString());
}

void TestCodeEditorSearch::documentPathFromCreationVariables() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("created.md"));
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
	file.write("# created");
	file.close();

	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	TestCommandController controller;

	auto* registry = controller.widgets();
	registry->registerFactory({ QStringLiteral("Test.CodeEditor"), QStringLiteral("code-editor"),
		/*singletone*/ false, QStringLiteral("Редактор") },
		[&highlighting, &formatters](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new CodeEditorDocument(&highlighting, &formatters, handler.parent);
		});

	MainWindow window(controller.globalContext(), nullptr, false);

	// Переменные создания записываются в контекст документа до его привязки.
	QVERIFY(window.createWidget(QStringLiteral("editor"), MainWindow::WidgetKind::Document,
		QStringLiteral("Редактор"), QStringLiteral("📝"),
		[registry] { return registry->build(QStringLiteral("code-editor"), WidgetBuildHandler()); },
		QString(), QStringLiteral("code-editor"), controller.globalContext(),
		{ { QStringLiteral("path"), path } }));

	// Открытие файла из onContextAttached() отложено до следующего цикла событий.
	QCoreApplication::processEvents();

	auto* document = qobject_cast<CodeEditorDocument*>(window.document(QStringLiteral("editor")));
	QVERIFY(document);
	QCOMPARE(document->filePath(), path);
	QCOMPARE(document->title(), QStringLiteral("created.md"));

	auto context = window.documentContext(QStringLiteral("editor"));
	QVERIFY(context);
	QCOMPARE(context->ownVariables().value(QStringLiteral("path")).toString(), path);
}

void TestCodeEditorSearch::dataVariablePopulatesEditor() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	TestCommandController controller;

	CodeEditorDocument document(&highlighting, &formatters);
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("editor"));
	QVERIFY(context);
	context->setVariable(QStringLiteral("data"), QStringLiteral("line1\nline2"));
	document.setContext(context);

	QCOMPARE(document.editor()->toPlainText(), QStringLiteral("line1\nline2"));
	QCOMPARE(document.title(), QStringLiteral("Данные"));
}

void TestCodeEditorSearch::saveDataUpdatesContextVariable() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	TestCommandController controller;

	CodeEditorDocument document(&highlighting, &formatters);
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("editor"));
	context->setVariable(QStringLiteral("data"), QStringLiteral("old"));
	document.setContext(context);

	QSignalSpy spy(context.get(), &CommandContext::eventFired);
	document.editor()->setPlainText(QStringLiteral("updated\nvalue"));
	QVERIFY(document.save());

	QCOMPARE(context->variable(QStringLiteral("data")).toString(),
		QStringLiteral("updated\nvalue"));
	QVERIFY(spy.count() >= 1);
	QCOMPARE(spy.last().at(0).toString(), CommandContext::documentSaveCompleteEvent());
}

void TestCodeEditorSearch::dataAndExistingFilePrefersFile() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("blob.txt"));
	{
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
		file.write("from file");
	}

	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	TestCommandController controller;

	CodeEditorDocument document(&highlighting, &formatters);
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("editor"));
	context->setVariable(QStringLiteral("path"), path);
	context->setVariable(QStringLiteral("data"), QStringLiteral("from context"));
	document.setContext(context);

	QTRY_COMPARE(document.editor()->toPlainText().trimmed(), QStringLiteral("from file"));

	document.editor()->setPlainText(QStringLiteral("saved"));
	QVERIFY(document.save());

	QFile file(path);
	QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
	QCOMPARE(QString::fromUtf8(file.readAll()), QStringLiteral("saved"));
	// Источник истины — файл: data не перезаписывается.
	QCOMPARE(context->variable(QStringLiteral("data")).toString(),
		QStringLiteral("from context"));
}

void TestCodeEditorSearch::dataAndMissingFileSavesBoth() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("new.txt"));
	QVERIFY(!QFileInfo::exists(path));

	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	TestCommandController controller;

	CodeEditorDocument document(&highlighting, &formatters);
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("editor"));
	context->setVariable(QStringLiteral("path"), path);
	context->setVariable(QStringLiteral("data"), QStringLiteral("from context"));
	document.setContext(context);

	QCOMPARE(document.editor()->toPlainText(), QStringLiteral("from context"));

	document.editor()->setPlainText(QStringLiteral("updated"));
	QVERIFY(document.save());

	QFile file(path);
	QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
	QCOMPARE(QString::fromUtf8(file.readAll()), QStringLiteral("updated"));
	QCOMPARE(context->variable(QStringLiteral("data")).toString(),
		QStringLiteral("updated"));
}

void TestCodeEditorSearch::documentMetadataDeclaresPath() {
	CodeEditorDocumentMetadata metadata;
	const auto variables = metadata.variables();
	QCOMPARE(variables.size(), 1);
	QCOMPARE(variables.first().name, QStringLiteral("path"));
	QCOMPARE(variables.first().mimeType,
		QString::fromLatin1(DocumentVariableMime::kFilePath));
}

void TestCodeEditorSearch::newDocumentDialogShowsCodeEditorPath() {
	TestCommandController controller;
	auto* registry = controller.widgets();
	registry->registerFactory({ QStringLiteral("Content.CodeEditor.CodeEditorDocument"),
		QStringLiteral("code-editor"), /*singletone*/ false,
		QStringLiteral("Редактор кода") },
		[](const QString&, const WidgetBuildHandler&) -> QWidget* {
			return nullptr;
		});
	registry->registerDocumentMetadata(
		QStringLiteral("Content.CodeEditor.CodeEditorDocument"),
		std::make_unique<CodeEditorDocumentMetadata>());

	NewDocumentDialog dialog(registry, controller.globalContext());

	auto* list = dialog.findChild<QListWidget*>(QStringLiteral("NewDocumentDialogTypes"));
	auto* properties = dialog.findChild<PropertiesListWidget*>(
		QStringLiteral("NewDocumentDialogVariables"));
	QVERIFY(list);
	QVERIFY(properties);

	// Обязательная переменная редактора — path; путь редактируется через
	// выбор файла (PropertyType::PathFile), поэтому поле только для чтения.
	QCOMPARE(list->count(), 1);
	QCOMPARE(dialog.documentType(), QStringLiteral("code-editor"));
	QCOMPARE(dialog.variables().size(), 1);
	QVERIFY(dialog.variables().contains(QStringLiteral("path")));

	QLineEdit* pathEdit = nullptr;
	for (auto* child : properties->findChildren<QWidget*>()) {
		if (child->property("propertyId").toString() != QStringLiteral("path")) {
			continue;
		}

		pathEdit = qobject_cast<QLineEdit*>(child);
		if (!pathEdit) {
			pathEdit = child->findChild<QLineEdit*>();
		}
	}
	QVERIFY(pathEdit);
	QVERIFY(pathEdit->isReadOnly());
}

void TestCodeEditorSearch::searchPropertiesGroupIsExposed() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	QWidget host;
	const auto groups = document.createPropertyGroups(&host);

	// Групп две: «Информация о документе» и «Параметры поиска».
	QCOMPARE(groups.size(), 2);
	QVERIFY(groupEditor(groups, QStringLiteral("info")));
	QVERIFY(groupEditor(groups, QStringLiteral("search")));

	auto* properties = qobject_cast<PropertiesListWidget*>(
		groupEditor(groups, QStringLiteral("search")));
	QVERIFY(properties);
	QVERIFY(!properties->isScrollable());
	QCOMPARE(properties->propertyCount(), 4);
	QVERIFY(properties->propertyIds().contains(QStringLiteral("search.regex")));
	QVERIFY(properties->propertyIds().contains(QStringLiteral("search.case-sensitive")));

	// Подписи сохраняют значки из ленты: .*, \n, Aa, ab.
	QStringList labels;
	for (auto* label : properties->findChildren<QLabel*>()) {
		labels << label->text();
	}
	QVERIFY(labels.contains(QStringLiteral(".*  Регулярное выражение")));
	QVERIFY(labels.contains(QStringLiteral("\\n  Упрощённое выражение")));
	QVERIFY(labels.contains(QStringLiteral("Aa  Учитывать регистр")));
	QVERIFY(labels.contains(QStringLiteral("ab  Слово целиком")));
}

void TestCodeEditorSearch::documentInfoGroupShowsCounts() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	document.editor()->setText(QStringLiteral("abc\ndefg\nh"));

	QWidget host;
	const auto groups = document.createPropertyGroups(&host);
	QWidget* info = groupEditor(groups, QStringLiteral("info"));
	QVERIFY(info);

	auto* lines = info->findChild<QLabel*>(QStringLiteral("CodeEditorInfoLines"));
	auto* characters = info->findChild<QLabel*>(QStringLiteral("CodeEditorInfoChars"));
	auto* longest = info->findChild<QLabel*>(QStringLiteral("CodeEditorInfoLongest"));
	QVERIFY(lines);
	QVERIFY(characters);
	QVERIFY(longest);

	QCOMPARE(lines->text(), QStringLiteral("3"));
	QCOMPARE(characters->text(), QStringLiteral("10"));
	QCOMPARE(longest->text(), QStringLiteral("4"));

	// После правки значения обновляются (символы и длинная строка — с задержкой).
	document.editor()->setText(QStringLiteral("a\nbbbbbb\ncc"));
	QTRY_COMPARE(longest->text(), QStringLiteral("6"));
	QCOMPARE(characters->text(), QStringLiteral("11"));
	QCOMPARE(lines->text(), QStringLiteral("3"));
}

void TestCodeEditorSearch::encodingComboIsSearchableAndApplied() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	QWidget host;
	QWidget* properties = document.createPropertiesEditor(&host);
	QVERIFY(properties);

	auto* combo = properties->findChild<QComboBox*>(QStringLiteral("CodeEditorEncodingCombo"));
	QVERIFY(combo);
	QVERIFY(combo->isEditable());
	QVERIFY(combo->completer());
	QCOMPARE(combo->completer()->filterMode(), Qt::MatchContains);
	QCOMPARE(combo->count(), CodeEditorWidget::availableEncodings().size());

	// Выбор кодировки в списке применяется к редактору.
	const int utf16Index = combo->findData(static_cast<int>(QStringConverter::Utf16));
	QVERIFY(utf16Index >= 0);
	combo->setCurrentIndex(utf16Index);
	QCOMPARE(document.editor()->encoding(), QStringConverter::Utf16);

	// Чтение и сохранение используют выбранную кодировку.
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("encoded.md"));
	{
		QStringEncoder encoder(QStringConverter::Utf16);
		const QByteArray bytes = encoder.encode(QStringLiteral("**привет**"));
		QFile file(path);
		QVERIFY(file.open(QIODevice::WriteOnly));
		file.write(bytes);
	}

	QVERIFY(document.openFile(path));
	QTRY_COMPARE(document.editor()->toPlainText().trimmed(), QStringLiteral("**привет**"));

	document.editor()->setText(QStringLiteral("**новый текст**"));
	QVERIFY(document.save());

	QFile saved(path);
	QVERIFY(saved.open(QIODevice::ReadOnly));
	const QByteArray savedBytes = saved.readAll();
	QStringDecoder decoder(QStringConverter::Utf16);
	const QString savedText = decoder.decode(savedBytes);
	QCOMPARE(savedText.trimmed(), QStringLiteral("**новый текст**"));
}

void TestCodeEditorSearch::searchPropertiesToggleModes() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	QWidget host;
	auto groups = document.createPropertyGroups(&host);
	auto* properties = qobject_cast<PropertiesListWidget*>(
		groupEditor(groups, QStringLiteral("search")));
	QVERIFY(properties);

	// Меняем флажок в панели свойств: документ передаёт его панели поиска
	// и хранит состояние для следующей перестройки панели.
	auto* regex = properties->findChild<QCheckBox*>();
	QVERIFY(regex);
	QVERIFY(!propertyValue(properties, QStringLiteral("search.regex")));
	QVERIFY(!propertyValue(properties, QStringLiteral("search.simplified")));

	// Включаем регулярное выражение.
	for (auto* child : properties->findChildren<QWidget*>()) {
		if (child->property("propertyId").toString() != QStringLiteral("search.regex")) {
			continue;
		}
		auto* checkBox = qobject_cast<QCheckBox*>(child);
		if (!checkBox) {
			checkBox = child->findChild<QCheckBox*>();
		}
		QVERIFY(checkBox);
		checkBox->setChecked(true);
		break;
	}

	groups = document.createPropertyGroups(&host);
	properties = qobject_cast<PropertiesListWidget*>(
		groupEditor(groups, QStringLiteral("search")));
	QVERIFY(properties);
	QVERIFY(propertyValue(properties, QStringLiteral("search.regex")));
	QVERIFY(!propertyValue(properties, QStringLiteral("search.simplified")));

	// Упрощённое выражение выключает регулярное: режимы взаимоисключающие.
	for (auto* child : properties->findChildren<QWidget*>()) {
		if (child->property("propertyId").toString() != QStringLiteral("search.simplified")) {
			continue;
		}
		auto* checkBox = qobject_cast<QCheckBox*>(child);
		if (!checkBox) {
			checkBox = child->findChild<QCheckBox*>();
		}
		QVERIFY(checkBox);
		checkBox->setChecked(true);
		break;
	}

	groups = document.createPropertyGroups(&host);
	properties = qobject_cast<PropertiesListWidget*>(
		groupEditor(groups, QStringLiteral("search")));
	QVERIFY(properties);
	QVERIFY(propertyValue(properties, QStringLiteral("search.simplified")));
	QVERIFY(!propertyValue(properties, QStringLiteral("search.regex")));
}

void TestCodeEditorSearch::ribbonSearchButtonControlsPanel() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	auto* searchPanel = document.findChild<CodeEditorSearchPanel*>();
	QVERIFY(searchPanel);
	QVERIFY(searchPanel->isHidden());

	auto* button = findRibbonButton(&document, QStringLiteral("search"));
	QVERIFY(button);
	QVERIFY(button->isCheckable());

	button->mainButton()->click();
	QVERIFY(!searchPanel->isHidden());
	QVERIFY(button->isChecked());

	button->mainButton()->click();
	QVERIFY(searchPanel->isHidden());
	QVERIFY(!button->isChecked());
}

void TestCodeEditorSearch::ribbonSearchButtonOpensProperties() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	auto* button = findRibbonButton(&document, QStringLiteral("search"));
	QVERIFY(button);

	// Значок свойств в правом нижнем углу сообщает группу «Параметры поиска».
	QVERIFY(button->propertiesButton());
	QSignalSpy spy(button, &RibbonButton::propertiesRequested);
	button->propertiesButton()->click();
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy.first().at(0).toString(), QStringLiteral("search"));
}

void TestCodeEditorSearch::propertiesVisibleOnFirstShow() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	TestCommandController controller;

	auto* registry = controller.widgets();
	registry->registerFactory({ QStringLiteral("Test.Panels.Properties"),
		QStringLiteral("properties"), /*singletone*/ true, QStringLiteral("Свойства") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new PropertiesPanel(handler.parent);
		});

	MainWindow window(controller.globalContext(), nullptr, false);
	QVERIFY(window.createWidget(QStringLiteral("properties"),
		MainWindow::WidgetKind::SidePanel, QStringLiteral("Свойства"), QStringLiteral("🛠"),
		[&registry] {
			return registry->build(QStringLiteral("properties"), WidgetBuildHandler());
		},
		QString(), QStringLiteral("properties"), controller.globalContext()));

	CodeEditorDocument document(&highlighting, &formatters);
	document.setTitle(QStringLiteral("Редактор"));
	document.setContext(controller.globalContext()->createDocumentContext(QStringLiteral("editor")));

	auto* panel = window.propertiesPanel();
	QVERIFY(panel);

	// Свойства строятся, пока панель скрыта, — как при первом открытии
	// документа в приложении.
	panel->setDocument(&document);
	QVERIFY(!panel->isVisible());
	QVERIFY(window.showDocumentProperties(&document));
	window.show();
	QCoreApplication::processEvents();

	// Группа «Свойства документа» раскрыта и содержит редакторы параметров.
	auto* language = panel->findChild<QComboBox*>(QStringLiteral("CodeEditorLanguageCombo"));
	QVERIFY(language);
	QVERIFY2(language->isVisible(), "язык не виден при первом показе свойств");
	QVERIFY2(language->height() > 0, "у языка нулевая высота при первом показе");

	// «Параметры поиска» раскрываются и тоже не пустые.
	panel->revealGroup(QStringLiteral("search"));
	QCoreApplication::processEvents();
	auto* search = panel->findChild<PropertiesListWidget*>(
		QStringLiteral("CodeEditorSearchProperties"));
	QVERIFY(search);
	QVERIFY(!search->isScrollable());
	QVERIFY2(search->isVisible(), "параметры поиска не видны после раскрытия");
	QVERIFY2(search->height() > 0, "у параметров поиска нулевая высота");
}

void TestCodeEditorSearch::smallDocumentHighlightsAllBlocks() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	document.resize(700, 500);
	document.show();
	QCoreApplication::processEvents();

	auto* editor = document.editor();
	editor->setLanguage(QStringLiteral("md"));

	QStringList lines;
	for (int i = 0; i < 50; i++) {
		lines << QStringLiteral("**жирная** строка %1").arg(i);
	}
	editor->setText(lines.join(QLatin1Char('\n')));

	QVERIFY(!editor->isHighlightingLazy());
	QCOMPARE(formattedBlocks(editor->document()), editor->document()->blockCount());
}

void TestCodeEditorSearch::largeDocumentHighlightsOnlyVisibleBlocks() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	document.resize(800, 600);
	document.show();
	QCoreApplication::processEvents();

	auto* editor = document.editor();
	editor->setLanguage(QStringLiteral("md"));

	QStringList lines;
	for (int i = 0; i < 6000; i++) {
		lines << QStringLiteral("**жирная** строка %1 с *курсивом*").arg(i);
	}
	editor->setText(lines.join(QLatin1Char('\n')));
	QVERIFY(editor->isHighlightingLazy());

	QTest::qWait(150);
	const int visibleFormatted = formattedBlocks(editor->document());
	QVERIFY2(visibleFormatted > 0, "видимые строки не подсвечены");
	QVERIFY2(visibleFormatted < 600,
		qPrintable(QStringLiteral("подсвечено строк: %1").arg(visibleFormatted)));

	// После прокрутки в конец подсветка должна появиться и там.
	editor->verticalScrollBar()->setValue(editor->verticalScrollBar()->maximum());
	QTest::qWait(200);

	const QTextBlock last = editor->document()->lastBlock();
	QVERIFY(last.layout());
	QVERIFY2(!last.layout()->formats().isEmpty(),
		"последняя строка не подсвечена после прокрутки");
}

void TestCodeEditorSearch::longLineDisablesHighlighting() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	document.resize(800, 600);
	document.show();
	QCoreApplication::processEvents();

	auto* editor = document.editor();
	editor->setLanguage(QStringLiteral("md"));

	// Обычный текст подсвечивается.
	editor->setText(QStringLiteral("**короткая** строка"));
	QVERIFY(editor->isHighlightingEnabled());
	QVERIFY(!editor->document()->firstBlock().layout()->formats().isEmpty());

	// Длинная строка отключает подсветку всего документа и сообщает об этом.
	QSignalSpy enabledChanged(editor, &CodeEditorWidget::highlightingEnabledChanged);
	const QString longLine = QStringLiteral("**жирно** ").repeated(600);
	editor->setText(longLine + QStringLiteral("\n**короткая** строка"));

	QVERIFY(!editor->isHighlightingEnabled());
	QCOMPARE(enabledChanged.count(), 1);
	QCOMPARE(enabledChanged.first().at(0).toBool(), false);

	// Форматы сняты со всех строк, включая короткую.
	const auto formatsEmpty = [](const QTextBlock& block) {
		return !block.layout() || block.layout()->formats().isEmpty();
	};
	const QTextBlock first = editor->document()->firstBlock();
	QVERIFY(formatsEmpty(first));
	QVERIFY(formatsEmpty(first.next()));

	// Как только длинная строка исчезла, подсветка включается снова.
	editor->setText(QStringLiteral("**короткая** строка"));
	QVERIFY(editor->isHighlightingEnabled());
	QCOMPARE(enabledChanged.count(), 2);
	QCOMPARE(enabledChanged.last().at(0).toBool(), true);
	QVERIFY(!editor->document()->firstBlock().layout()->formats().isEmpty());
}

void TestCodeEditorSearch::longLineSuppressionIsReportedToConsole() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	TestCommandController controller;
	CodeEditorDocument document(&highlighting, &formatters);
	document.resize(800, 600);
	document.show();
	QCoreApplication::processEvents();

	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("editor"));
	QVERIFY(context);
	document.setContext(context);
	document.editor()->setLanguage(QStringLiteral("md"));

	QSignalSpy output(&controller, &CommandController::outputRequested);

	// Длинная строка выключает подсветку — документ сообщает об этом.
	document.editor()->setText(QString(QStringLiteral("x")).repeated(
		CodeEditorWidget::maxHighlightedLineLength() + 100));
	QCoreApplication::processEvents();

	QVERIFY(!document.editor()->isHighlightingEnabled());
	bool warnedOff = false;
	for (const auto& args : output) {
		if (args.at(0).toString().contains(QStringLiteral("Подсветка отключена"))) {
			warnedOff = true;
		}
	}
	QVERIFY2(warnedOff, "нет предупреждения об отключении подсветки");

	// Строка стала короткой — подсветка вернулась, тоже с сообщением.
	output.clear();
	document.editor()->setText(QStringLiteral("short line"));
	QCoreApplication::processEvents();

	QVERIFY(document.editor()->isHighlightingEnabled());
	bool notifiedOn = false;
	for (const auto& args : output) {
		if (args.at(0).toString().contains(QStringLiteral("Подсветка снова включена"))) {
			notifiedOn = true;
		}
	}
	QVERIFY2(notifiedOn, "нет сообщения о включении подсветки");
}

void TestCodeEditorSearch::manualHighlightingOverridesLongLines() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	document.resize(800, 600);
	document.show();
	QCoreApplication::processEvents();

	auto* editor = document.editor();
	editor->setLanguage(QStringLiteral("md"));

	const QString longLine = QStringLiteral("**жирно** ").repeated(600);
	editor->setText(longLine + QStringLiteral("\n**короткая** строка"));
	QVERIFY(!editor->isHighlightingEnabled());

	const auto formatsEmpty = [](const QTextBlock& block) {
		return !block.layout() || block.layout()->formats().isEmpty();
	};

	// Ручное включение имеет приоритет: короткие строки снова подсвечены,
	// хотя сама длинная строка остаётся без форматов (защита от зависания).
	editor->setHighlightingPolicy(CodeEditorWidget::HighlightingPolicy::On);
	QCOMPARE(editor->highlightingPolicy(), CodeEditorWidget::HighlightingPolicy::On);
	QVERIFY(editor->isHighlightingEnabled());
	QVERIFY(formatsEmpty(editor->document()->firstBlock()));
	QVERIFY(!editor->document()->firstBlock().next().layout()->formats().isEmpty());

	// Ручное выключение снимает форматы со всех строк.
	editor->setHighlightingPolicy(CodeEditorWidget::HighlightingPolicy::Off);
	QCOMPARE(editor->highlightingPolicy(), CodeEditorWidget::HighlightingPolicy::Off);
	QVERIFY(!editor->isHighlightingEnabled());
	QVERIFY(formatsEmpty(editor->document()->firstBlock()));
	QVERIFY(formatsEmpty(editor->document()->firstBlock().next()));

	// Возврат в «авто» с длинной строкой снова выключает подсветку.
	editor->setHighlightingPolicy(CodeEditorWidget::HighlightingPolicy::Auto);
	QVERIFY(!editor->isHighlightingEnabled());
}

void TestCodeEditorSearch::manualHighlightingIsReportedToConsole() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	TestCommandController controller;
	CodeEditorDocument document(&highlighting, &formatters);
	auto context = controller.globalContext()->createDocumentContext(QStringLiteral("editor"));
	QVERIFY(context);
	document.setContext(context);
	document.editor()->setLanguage(QStringLiteral("md"));

	// Документ с длинной строкой: авто-режим подсветку выключает.
	document.editor()->setText(QStringLiteral("x").repeated(
		CodeEditorWidget::maxHighlightedLineLength() + 100));
	QCoreApplication::processEvents();
	QVERIFY(!document.editor()->isHighlightingEnabled());

	// Ручное включение: подсветка включается, но с предупреждением о тормозах.
	QSignalSpy output(&controller, &CommandController::outputRequested);
	document.editor()->setHighlightingPolicy(CodeEditorWidget::HighlightingPolicy::On);
	QCoreApplication::processEvents();

	QVERIFY(document.editor()->isHighlightingEnabled());
	bool warnedManual = false;
	for (const auto& args : output) {
		if (args.at(0).toString().contains(QStringLiteral("включена вручную"))) {
			warnedManual = true;
		}
	}
	QVERIFY2(warnedManual, "нет предупреждения о ручном включении подсветки");
}

void TestCodeEditorSearch::ribbonHighlightButtonToggles() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	document.resize(800, 600);
	document.show();
	QCoreApplication::processEvents();

	auto* editor = document.editor();
	editor->setLanguage(QStringLiteral("md"));

	auto* button = document.findChild<RibbonButton*>(
		QStringLiteral("CodeEditorHighlightButton"));
	QVERIFY(button);
	QVERIFY(button->isCheckable());
	QVERIFY(button->isChecked());

	// Авто-запрет из-за длинной строки снимает галочку.
	editor->setText(QStringLiteral("**жирно** ").repeated(600)
		+ QStringLiteral("\n**короткая** строка"));
	QVERIFY(!button->isChecked());

	// Ручное включение кнопкой работает даже при длинной строке.
	button->mainButton()->click();
	QVERIFY(button->isChecked());
	QCOMPARE(editor->highlightingPolicy(), CodeEditorWidget::HighlightingPolicy::On);
	QVERIFY(editor->isHighlightingEnabled());

	// Ручное выключение кнопкой.
	button->mainButton()->click();
	QVERIFY(!button->isChecked());
	QCOMPARE(editor->highlightingPolicy(), CodeEditorWidget::HighlightingPolicy::Off);
	QVERIFY(!editor->isHighlightingEnabled());
}

void TestCodeEditorSearch::newTextResetsHighlightingPolicy() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	auto* editor = document.editor();
	editor->setLanguage(QStringLiteral("md"));

	editor->setHighlightingPolicy(CodeEditorWidget::HighlightingPolicy::Off);
	QCOMPARE(editor->highlightingPolicy(), CodeEditorWidget::HighlightingPolicy::Off);

	// Замена текста — новый документ, снова работает авто-режим.
	editor->setText(QStringLiteral("**короткая** строка"));
	QCOMPARE(editor->highlightingPolicy(), CodeEditorWidget::HighlightingPolicy::Auto);
	QVERIFY(editor->isHighlightingEnabled());

	// Открытие файла тоже сбрасывает ручной выбор.
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const auto path = dir.filePath(QStringLiteral("sample.md"));
	QFile file(path);
	QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
	file.write("# sample");
	file.close();

	editor->setHighlightingPolicy(CodeEditorWidget::HighlightingPolicy::Off);
	QVERIFY(document.openFile(path));
	QTRY_COMPARE(editor->toPlainText().trimmed(), QStringLiteral("# sample"));
	QCOMPARE(editor->highlightingPolicy(), CodeEditorWidget::HighlightingPolicy::Auto);
}

void TestCodeEditorSearch::longLinesEnableAutoWrap() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	auto* editor = document.editor();
	QCOMPARE(editor->lineWrapPolicy(), CodeEditorWidget::LineWrapPolicy::Auto);
	QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::NoWrap);

	// Строка длиннее предела включает перенос: без него QPlainTextEdit
	// пересчитывает всю строку на каждый кадр.
	const QString longLine(CodeEditorWidget::autoWrapLineLimit() + 100, QLatin1Char('x'));
	editor->setText(longLine);
	QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::WidgetWidth);
	QVERIFY(editor->hasLongLines());

	// Короткий текст возвращает обычный режим.
	editor->setText(QStringLiteral("short line"));
	QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::NoWrap);
	QVERIFY(!editor->hasLongLines());

	// Явно включённый перенос работает и на коротких строках — именно этого
	// не делал прежний флажок «автоперенос».
	editor->setLineWrapPolicy(CodeEditorWidget::LineWrapPolicy::On);
	QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::WidgetWidth);

	// Выключенный режим возвращает NoWrap для обычных длинных строк...
	editor->setLineWrapPolicy(CodeEditorWidget::LineWrapPolicy::Off);
	editor->setText(longLine);
	QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::NoWrap);

	// ...и даже очень длинную строку не переносим: приоритет у пользователя.
	const QString hugeLine(CodeEditorWidget::maxHighlightedLineLength() + 100, QLatin1Char('y'));
	editor->setText(hugeLine);
	QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::NoWrap);

	// Обратно в «авто» — перенос снова включается по длине строки.
	editor->setLineWrapPolicy(CodeEditorWidget::LineWrapPolicy::Auto);
	QCOMPARE(editor->lineWrapMode(), QPlainTextEdit::WidgetWidth);
}

void TestCodeEditorSearch::lineWrapComboControlsWrap() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	document.editor()->setText(QStringLiteral("short line"));

	QWidget host;
	QWidget* properties = document.createPropertiesEditor(&host);
	QVERIFY(properties);

	auto* combo = properties->findChild<QComboBox*>(QStringLiteral("CodeEditorLineWrapCombo"));
	QVERIFY(combo);
	QCOMPARE(combo->count(), 3);

	// «Выключен» и «Включён» действуют независимо от длины строк.
	combo->setCurrentIndex(combo->findData(
		static_cast<int>(CodeEditorWidget::LineWrapPolicy::On)));
	QCOMPARE(document.editor()->lineWrapMode(), QPlainTextEdit::WidgetWidth);

	combo->setCurrentIndex(combo->findData(
		static_cast<int>(CodeEditorWidget::LineWrapPolicy::Off)));
	QCOMPARE(document.editor()->lineWrapMode(), QPlainTextEdit::NoWrap);
}

void TestCodeEditorSearch::formatAppliesResultAndKeepsCursor() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	formatters.registerPlugin(std::make_unique<TestFormatter>(TestFormatter::Mode::Success));

	CodeEditorDocument document(&highlighting, &formatters);
	document.resize(700, 500);
	document.show();
	QCoreApplication::processEvents();

	auto* editor = document.editor();
	editor->setLanguage(QStringLiteral("md"));
	editor->setText(QStringLiteral("line bad\nsecond bad\nthird bad"));

	QTextCursor cursor = editor->textCursor();
	cursor.movePosition(QTextCursor::End);
	editor->setTextCursor(cursor);
	const int position = cursor.position();

	QSignalSpy started(editor, &CodeEditorWidget::formattingStarted);
	QSignalSpy completed(editor, &CodeEditorWidget::formattingCompleted);

	QVERIFY(editor->formatDocument());
	QCOMPARE(started.count(), 1);
	QCOMPARE(completed.count(), 1);
	QCOMPARE(editor->toPlainText(), QStringLiteral("line good\nsecond good\nthird good"));
	QCOMPARE(editor->textCursor().position(), position);
	QVERIFY(editor->document()->isModified());
}

void TestCodeEditorSearch::formatFailureIsReported() {
	Resources resources;
	resources.Directories.set(QStringLiteral("path.theme"), QDir(themeDirectory()));
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);
	formatters.registerPlugin(std::make_unique<TestFormatter>(TestFormatter::Mode::Failure));

	CodeEditorDocument document(&highlighting, &formatters);
	auto* editor = document.editor();
	editor->setLanguage(QStringLiteral("md"));
	editor->setText(QStringLiteral("line bad"));

	QSignalSpy failed(editor, &CodeEditorWidget::formattedFailed);
	QSignalSpy completed(editor, &CodeEditorWidget::formattingCompleted);

	QVERIFY(editor->formatDocument());
	QCOMPARE(failed.count(), 1);
	QCOMPARE(completed.count(), 1);
	QVERIFY(!editor->isFormatting());
	QCOMPARE(editor->toPlainText(), QStringLiteral("line bad"));
}

QTEST_MAIN(TestCodeEditorSearch)
#include "test_code_editor_search.moc"
