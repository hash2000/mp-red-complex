#include "Content/CodeEditorWidget/documents/code_editor_document.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/widgets/code_editor_search_panel.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Resources/resources.h"

#include <QDir>
#include <QFile>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QtTest/QtTest>

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
		/*singletone*/ false, QStringLiteral("📝"), QStringLiteral("Редактор") },
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

QTEST_MAIN(TestCodeEditorSearch)
#include "test_code_editor_search.moc"