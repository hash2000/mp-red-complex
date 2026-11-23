#include "Content/CodeEditorWidget/documents/code_editor_document.h"
#include "Content/CodeEditorWidget/documents/code_editor_document_metadata.h"
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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTextEdit>
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
} // namespace

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
	void searchPropertiesToggleModes();
	void ribbonSearchButtonControlsPanel();
	void ribbonSearchButtonOpensProperties();
	void propertiesVisibleOnFirstShow();

private:
	QTextEdit* _editor = nullptr;
	CodeEditorSearchPanel* _panel = nullptr;
};

void TestCodeEditorSearch::init() {
	_editor = new QTextEdit;
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
	_editor->setLineWrapMode(QTextEdit::NoWrap);
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

	QCOMPARE(groups.size(), 1);
	QCOMPARE(groups.first().id, QStringLiteral("search"));
	QCOMPARE(groups.first().title, QStringLiteral("Параметры поиска"));

	auto* properties = qobject_cast<PropertiesListWidget*>(groups.first().editor);
	QVERIFY(properties);
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

void TestCodeEditorSearch::searchPropertiesToggleModes() {
	Resources resources;
	HighlightingPluginManager highlighting(&resources);
	FormatterPluginManager formatters(&resources);

	CodeEditorDocument document(&highlighting, &formatters);
	QWidget host;
	auto groups = document.createPropertyGroups(&host);
	auto* properties = qobject_cast<PropertiesListWidget*>(groups.first().editor);
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
	properties = qobject_cast<PropertiesListWidget*>(groups.first().editor);
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
	properties = qobject_cast<PropertiesListWidget*>(groups.first().editor);
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
	QVERIFY2(search->isVisible(), "параметры поиска не видны после раскрытия");
	QVERIFY2(search->height() > 0, "у параметров поиска нулевая высота");
}

QTEST_MAIN(TestCodeEditorSearch)
#include "test_code_editor_search.moc"
