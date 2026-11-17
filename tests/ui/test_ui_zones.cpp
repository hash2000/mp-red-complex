#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/commands/widget_zone_command.h"

#include <QLabel>
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

/// Регистрирует простой виджет и считает вызовы фабрики.
struct WidgetFixture {
	int built = 0;

	explicit WidgetFixture(CommandController& controller) {
		controller.widgets()->registerFactory({ "Test.Widget", "test-widget" },
			[this](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
				built++;
				auto* label = new QLabel(QStringLiteral("test"), handler.parent);
				label->setObjectName(QStringLiteral("TestWidget"));
				return label;
			});
	}
};

std::shared_ptr<CommandContext> commandContext(CommandController& controller,
	const QMap<QString, QVariant>& variables) {
	static int counter = 0;
	auto context = controller.globalContext()->createScopedContext(
		QStringLiteral("command-%1").arg(++counter));
	for (auto it = variables.cbegin(); it != variables.cend(); it++) {
		context->setVariable(it.key(), it.value());
	}
	return context;
}

} // namespace

class TestUiZones : public QObject {
	Q_OBJECT

private slots:
	void createWidgetInEveryZone();
	void repeatedIdReusesWidget();
	void idCannotMoveBetweenZones();
	void closeRemovesWidget();
	void declarativePanelKeepsType();
	void showAndListActions();
	void commandRunsThroughProcessor();
	void commandValidatesParameters();
	void widgetGetsDocumentContext();
	void documentCreatedFromDocumentNests();
	void variableChangeIsNotified();
	void documentAttachmentIsNotified();
	void documentActivationIsNotified();
	void existingDocumentInvocationIsNotified();
};

void TestUiZones::createWidgetInEveryZone() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);
	QCOMPARE(controller.mainWindow(), &window);

	QWidget* document = window.createWidget(QStringLiteral("doc"),
		MainWindow::WidgetKind::Document, QStringLiteral("Документ"),
		QStringLiteral("#"), [&fixture] { return fixture.built++, new QLabel(QStringLiteral("doc")); },
		QString(), QStringLiteral("test-widget"));
	QWidget* bottom = window.createWidget(QStringLiteral("info"),
		MainWindow::WidgetKind::BottomPanel, QStringLiteral("Вывод"),
		QStringLiteral(">"), [&fixture] { return fixture.built++, new QLabel(QStringLiteral("info")); },
		QString(), QStringLiteral("test-widget"));
	QWidget* side = window.createWidget(QStringLiteral("side"),
		MainWindow::WidgetKind::SidePanel, QStringLiteral("Свойства"),
		QStringLiteral("*"), [&fixture] { return fixture.built++, new QLabel(QStringLiteral("side")); },
		QString(), QStringLiteral("test-widget"));

	QVERIFY(document);
	QVERIFY(bottom);
	QVERIFY(side);
	QCOMPARE(fixture.built, 3);
	QCOMPARE(window.widgetKind(QStringLiteral("doc")), std::optional<MainWindow::WidgetKind>(MainWindow::WidgetKind::Document));
	QCOMPARE(window.widgetKind(QStringLiteral("info")), std::optional<MainWindow::WidgetKind>(MainWindow::WidgetKind::BottomPanel));
	QCOMPARE(window.widgetKind(QStringLiteral("side")), std::optional<MainWindow::WidgetKind>(MainWindow::WidgetKind::SidePanel));
	QCOMPARE(window.widgetType(QStringLiteral("doc")), QStringLiteral("test-widget"));
	QCOMPARE(window.documentArea()->documents().size(), 1);
}

void TestUiZones::repeatedIdReusesWidget() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	QWidget* first = window.createWidget(QStringLiteral("singleton"),
		MainWindow::WidgetKind::Document, QStringLiteral("Первый"), QString(),
		[&fixture] { return fixture.built++, new QLabel; });
	QWidget* second = window.createWidget(QStringLiteral("singleton"),
		MainWindow::WidgetKind::Document, QStringLiteral("Второй"), QString(),
		[&fixture] { return fixture.built++, new QLabel; });

	QVERIFY(first);
	QCOMPARE(second, first);
	QCOMPARE(fixture.built, 1);
	QCOMPARE(window.widgetTitle(QStringLiteral("singleton")), QStringLiteral("Первый"));
}

void TestUiZones::idCannotMoveBetweenZones() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	WidgetZoneCommand documents(QStringLiteral("documents"), MainWindow::WidgetKind::Document);
	WidgetZoneCommand infoPanels(QStringLiteral("info-panels"), MainWindow::WidgetKind::BottomPanel);

	QVERIFY(documents.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "test-widget" }, { "id", "same" }
	})));
	QVERIFY(window.widget("same"));

	// Тот же id в другой области не создаётся и не подменяет виджет.
	QVERIFY(!infoPanels.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "test-widget" }, { "id", "same" }
	})));
	QCOMPARE(window.widgetKind("same"), std::optional<MainWindow::WidgetKind>(MainWindow::WidgetKind::Document));
	QCOMPARE(fixture.built, 1);
}

void TestUiZones::closeRemovesWidget() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	WidgetZoneCommand documents(QStringLiteral("documents"), MainWindow::WidgetKind::Document);
	QVERIFY(documents.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "test-widget" }, { "id", "doc-1" }
	})));
	QCOMPARE(window.widgetIds().size(), 1);

	QVERIFY(documents.execute(commandContext(controller, { { "action", "close" }, { "id", "doc-1" } })));
	QVERIFY(!window.widget("doc-1"));
	QVERIFY(window.widgetIds().isEmpty());
	QVERIFY(window.documentArea()->documents().isEmpty());
}

void TestUiZones::declarativePanelKeepsType() {
	TestCommandController controller;
	MainWindow window(controller.globalContext(), nullptr, false);

	auto* panel = new QLabel;
	window.registerBottomPanel(QStringLiteral("console"), QStringLiteral(">"),
		QStringLiteral("Консоль"), panel, QString(), QStringLiteral("console"));

	QCOMPARE(window.widget(QLatin1String("console")), panel);
	QCOMPARE(window.widgetType(QStringLiteral("console")), QStringLiteral("console"));
	QCOMPARE(window.widgetKind(QStringLiteral("console")),
		std::optional<MainWindow::WidgetKind>(MainWindow::WidgetKind::BottomPanel));
}

void TestUiZones::showAndListActions() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	WidgetZoneCommand documents(QStringLiteral("documents"), MainWindow::WidgetKind::Document);
	WidgetZoneCommand infoPanels(QStringLiteral("info-panels"), MainWindow::WidgetKind::BottomPanel);

	QVERIFY(documents.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "test-widget" }, { "id", "doc-show" }, { "title", "Заголовок" }
	})));
	QVERIFY(documents.execute(commandContext(controller, { { "action", "show" }, { "id", "doc-show" } })));
	QVERIFY(documents.execute(commandContext(controller, { { "action", "list" } })));
	// Чужая область не должна активировать этот id.
	QVERIFY(!infoPanels.execute(commandContext(controller, { { "action", "show" }, { "id", "doc-show" } })));
	QVERIFY(!documents.execute(commandContext(controller, { { "action", "show" }, { "id", "missing" } })));
	QCOMPARE(window.widgetTitle(QStringLiteral("doc-show")), QStringLiteral("Заголовок"));
}

void TestUiZones::commandRunsThroughProcessor() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	CommandProcessor processor(nullptr);
	processor.registerCommand(std::make_unique<WidgetZoneCommand>(
		QStringLiteral("documents"), MainWindow::WidgetKind::Document));
	QVERIFY(processor.findCommand(QStringLiteral("documents")));

	auto caller = controller.globalContext()->createScopedContext(QStringLiteral("console"));

	// Текстовый вызов проходит парсер и создаёт документ.
	QVERIFY(processor.execute(
		QStringLiteral("documents action=create, target=test-widget, id=parsed-id, title=\"Из консоли\""),
		caller));
	QVERIFY(window.widget(QStringLiteral("parsed-id")));
	QCOMPARE(window.widgetTitle(QStringLiteral("parsed-id")), QStringLiteral("Из консоли"));

	// Программный вызов с параметрами закрывает его.
	QVERIFY(processor.executeCommand(QStringLiteral("documents"),
		{ { QStringLiteral("action"), QStringLiteral("close") },
		  { QStringLiteral("id"), QStringLiteral("parsed-id") } }, caller));
	QVERIFY(!window.widget(QStringLiteral("parsed-id")));
	QCOMPARE(processor.availableCommands(), QStringList{ QStringLiteral("documents") });
}

void TestUiZones::commandValidatesParameters() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	WidgetZoneCommand documents(QStringLiteral("documents"), MainWindow::WidgetKind::Document);

	// Без target создать нельзя.
	QVERIFY(!documents.execute(commandContext(controller, { { "action", "create" } })));
	// Неизвестный target — фабрика не найдена.
	QVERIFY(!documents.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "unknown-widget" }, { "id", "x" }
	})));
	// Закрытие неизвестного id.
	QVERIFY(!documents.execute(commandContext(controller, { { "action", "close" }, { "id", "x" } })));
	// Неизвестное действие.
	QVERIFY(!documents.execute(commandContext(controller, { { "action", "destroy" } })));

	QVERIFY(window.widgetIds().isEmpty());
}

void TestUiZones::widgetGetsDocumentContext() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	auto* document = window.createWidget(QStringLiteral("doc"),
		MainWindow::WidgetKind::Document, QStringLiteral("Документ"), QString(),
		[&fixture] { return fixture.built++, new QLabel(QStringLiteral("doc")); },
		QString(), QStringLiteral("test-widget"));
	QVERIFY(document);

	// Любой виджет любой зоны — документ со своим контекстом.
	auto* panel = window.createWidget(QStringLiteral("side"),
		MainWindow::WidgetKind::SidePanel, QStringLiteral("Панель"), QString(),
		[&fixture] { return fixture.built++, new QLabel(QStringLiteral("side")); },
		QString(), QStringLiteral("test-widget"));
	QVERIFY(panel);

	auto documentContext = window.documentContext(QStringLiteral("doc"));
	auto panelContext = window.documentContext(QStringLiteral("side"));
	QVERIFY(documentContext);
	QVERIFY(panelContext);
	QVERIFY(documentContext->isDocumentContext());
	QCOMPARE(documentContext->documentId(), QStringLiteral("doc"));
	QCOMPARE(panelContext->documentId(), QStringLiteral("side"));
	QCOMPARE(documentContext->nameSpace(), QStringLiteral(".documents.doc"));
	// Документы ищутся среди документов своего создателя.
	QVERIFY(controller.globalContext()->documentContext(QStringLiteral("doc")));
	QCOMPARE(controller.documentsContext()->name(), QStringLiteral("documents"));
	QCOMPARE(window.document(QStringLiteral("doc"))->context(), documentContext);
}

void TestUiZones::documentCreatedFromDocumentNests() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	auto* explorer = window.createWidget(QStringLiteral("explorer"),
		MainWindow::WidgetKind::SidePanel, QStringLiteral("Проводник"), QString(),
		[&fixture] { return fixture.built++, new QLabel(QStringLiteral("explorer")); },
		QString(), QStringLiteral("test-widget"));
	QVERIFY(explorer);

	auto explorerContext = window.documentContext(QStringLiteral("explorer"));
	QVERIFY(explorerContext);

	// Документ, созданный документом, попадает в его контекст документов.
	WidgetZoneCommand documents(QStringLiteral("documents"), MainWindow::WidgetKind::Document);
	auto caller = explorerContext->createScopedContext(QStringLiteral("command"));
	caller->setVariable(QStringLiteral("action"), QStringLiteral("create"));
	caller->setVariable(QStringLiteral("target"), QStringLiteral("test-widget"));
	caller->setVariable(QStringLiteral("id"), QStringLiteral("editor"));
	QVERIFY(documents.execute(caller));

	auto editorContext = window.documentContext(QStringLiteral("editor"));
	QVERIFY(editorContext);
	QCOMPARE(editorContext->nameSpace(),
		QStringLiteral(".documents.explorer.documents.editor"));
	QCOMPARE(editorContext->owningDocumentContext(), explorerContext);
}

void TestUiZones::variableChangeIsNotified() {
	TestCommandController controller;
	auto context = controller.globalContext()->createScopedContext(QStringLiteral("view"));
	QSignalSpy spy(context.get(), &CommandContext::variableChanged);

	QVERIFY(context->setVariable(QStringLiteral("zoom"), 1));
	QCOMPARE(spy.count(), 1);

	const auto args = spy.takeFirst();
	QCOMPARE(args.at(0).toString(), QStringLiteral(".view"));
	QCOMPARE(args.at(1).toString(), QStringLiteral("zoom"));
	QVERIFY(!args.at(2).isValid());        // старого значения не было
	QCOMPARE(args.at(3).toInt(), 1);

	// Повторная запись того же значения не считается изменением.
	QVERIFY(context->setVariable(QStringLiteral("zoom"), 1));
	QCOMPARE(spy.count(), 0);

	QVERIFY(context->setVariable(QStringLiteral("zoom"), 2));
	QCOMPARE(spy.count(), 1);
	const auto changed = spy.takeFirst();
	QCOMPARE(changed.at(2).toInt(), 1);
	QCOMPARE(changed.at(3).toInt(), 2);
}

void TestUiZones::documentAttachmentIsNotified() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	auto documentsContext = controller.documentsContext();
	QVERIFY(documentsContext);
	QSignalSpy spy(documentsContext.get(), &CommandContext::documentAttached);

	QVERIFY(window.createWidget(QStringLiteral("doc"),
		MainWindow::WidgetKind::Document, QStringLiteral("Документ"), QString(),
		[&fixture] { return fixture.built++, new QLabel; }, QString(), QStringLiteral("test-widget")));

	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy.at(0).at(0).toString(), QStringLiteral("doc"));
}

void TestUiZones::documentActivationIsNotified() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	QVERIFY(window.createWidget(QStringLiteral("one"),
		MainWindow::WidgetKind::Document, QStringLiteral("Первый"), QString(),
		[&fixture] { return fixture.built++, new QLabel; }));
	QVERIFY(window.createWidget(QStringLiteral("two"),
		MainWindow::WidgetKind::Document, QStringLiteral("Второй"), QString(),
		[&fixture] { return fixture.built++, new QLabel; }));

	auto first = window.documentContext(QStringLiteral("one"));
	auto second = window.documentContext(QStringLiteral("two"));
	QVERIFY(first);
	QVERIFY(second);

	QSignalSpy firstActivated(first.get(), &CommandContext::documentActivated);
	QSignalSpy firstDeactivated(first.get(), &CommandContext::documentDeactivated);
	QSignalSpy secondActivated(second.get(), &CommandContext::documentActivated);

	window.showWidget(QStringLiteral("one"));
	QCOMPARE(window.activeDocumentId(), QStringLiteral("one"));
	QCOMPARE(firstActivated.count(), 1);

	window.showWidget(QStringLiteral("two"));
	QCOMPARE(window.activeDocumentId(), QStringLiteral("two"));
	QCOMPARE(firstDeactivated.count(), 1);
	QCOMPARE(secondActivated.count(), 1);
}

void TestUiZones::existingDocumentInvocationIsNotified() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	QVERIFY(window.createWidget(QStringLiteral("logger"),
		MainWindow::WidgetKind::BottomPanel, QStringLiteral("Журнал"), QString(),
		[&fixture] { return fixture.built++, new QLabel; }));

	auto loggerContext = window.documentContext(QStringLiteral("logger"));
	QVERIFY(loggerContext);
	QSignalSpy invoked(loggerContext.get(), &CommandContext::documentInvoked);

	// Повторное создание существующего документа — это вызов его другим документом.
	QVERIFY(window.createWidget(QStringLiteral("logger"),
		MainWindow::WidgetKind::BottomPanel, QStringLiteral("Журнал"), QString(),
		[&fixture] { return fixture.built++, new QLabel; },
		QString(), QString(), controller.documentsContext()));

	QCOMPARE(fixture.built, 1);   // фабрика повторно не вызывается
	QCOMPARE(invoked.count(), 1);
	QCOMPARE(invoked.at(0).at(0).toString(), QStringLiteral("logger"));
}

QTEST_MAIN(TestUiZones)
#include "test_ui_zones.moc"
