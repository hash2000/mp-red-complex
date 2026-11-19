#include "Libs/BaseWidgets/properties/properties_list_widget.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/main_window/new_document_dialog.h"
#include "Libs/Engine/ui/doc_widget_stub.h"
#include "Libs/Engine/scripting/widgets/i_document_metadata.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/commands/widget_zone_command.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDialog>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QSpinBox>
#include <QTimer>
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

/// Регистрирует простой виджет и считает вызовы фабрики.
struct WidgetFixture {
	int built = 0;

	explicit WidgetFixture(CommandController& controller) {
		controller.widgets()->registerFactory({ "Test.Widget", "test-widget", /*singletone*/ false,
			QStringLiteral("🧪"), QStringLiteral("Тестовый виджет") },
			[this](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
				built++;
				auto* label = new QLabel(QStringLiteral("test"), handler.parent);
				label->setObjectName(QStringLiteral("TestWidget"));
				return label;
			});
	}
};

/// Метаданные тестового документа: по переменной каждого типа редактора.
class TestDocumentMetadata final : public IDocumentMetadata {
public:
	QList<DocumentVariable> variables() const override {
		return {
			{ QStringLiteral("path"), QString::fromLatin1(DocumentVariableMime::kFilePath) },
			{ QStringLiteral("count"), QString::fromLatin1(DocumentVariableMime::kInteger) },
			{ QStringLiteral("enabled"), QString::fromLatin1(DocumentVariableMime::kBoolean) },
			{ QStringLiteral("note"), QString::fromLatin1(DocumentVariableMime::kText) }
		};
	}
};

/// Редактор свойства в PropertiesListWidget по идентификатору переменной.
template<typename T>
T* findPropertyEditor(PropertiesListWidget* properties, const QString& id) {
	for (auto* child : properties->findChildren<QWidget*>()) {
		if (child->property("propertyId").toString() != id) {
			continue;
		}

		if (auto* editor = qobject_cast<T*>(child)) {
			return editor;
		}
		if (auto* nested = child->findChild<T*>()) {
			return nested;
		}
	}

	return nullptr;
}

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
	void registryDescribesTypes();
	void singletoneDocumentKeepsXtypeName();
	void singletoneCreateWidgetNormalizesId();
	void commandUsesRegistryDefaults();
	void documentStateIsStoredInContext();
	void commandParametersBecomeDocumentVariables();
	void newDocumentDialogFiltersTypes();
	void newDocumentDialogUsesDocumentMetadata();
	void propertiesListWidgetAddsAndRemoves();
	void registryBuildWrapsWidgetIntoDocument();
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

void TestUiZones::registryDescribesTypes() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	controller.widgets()->registerFactory({ "Test.Singleton", "test-singleton", /*singletone*/ true,
		QStringLiteral("★"), QStringLiteral("Одиночный") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new QLabel(handler.title, handler.parent);
		});

	const auto types = controller.widgets()->types();
	QCOMPARE(types.size(), 2);
	QCOMPARE(types.at(0).xtype, QStringLiteral("test-singleton"));
	QCOMPARE(types.at(0).className, QStringLiteral("Test.Singleton"));
	QVERIFY(types.at(0).singletone);
	QCOMPARE(types.at(0).glyph, QStringLiteral("★"));
	QCOMPARE(types.at(0).title, QStringLiteral("Одиночный"));

	QCOMPARE(types.at(1).xtype, QStringLiteral("test-widget"));
	QVERIFY(!types.at(1).singletone);
	QCOMPARE(types.at(1).glyph, QStringLiteral("🧪"));
	QCOMPARE(types.at(1).title, QStringLiteral("Тестовый виджет"));

	// Поиск описания по xtype и по полному имени класса.
	const auto byClass = controller.widgets()->typeInfo(QStringLiteral("Test.Singleton"));
	QVERIFY(byClass);
	QCOMPARE(byClass->xtype, QStringLiteral("test-singleton"));
	QVERIFY(!controller.widgets()->typeInfo(QStringLiteral("unknown")));

	// Для singletone id всегда равен xtype, иначе сохраняется предложенный.
	QCOMPARE(controller.widgets()->documentId(QStringLiteral("test-singleton"),
		QStringLiteral("custom")), QStringLiteral("test-singleton"));
	QCOMPARE(controller.widgets()->documentId(QStringLiteral("test-widget"),
		QStringLiteral("custom")), QStringLiteral("custom"));

	// Метаданные документа ищутся и по xtype, и по полному имени класса.
	QVERIFY(!controller.widgets()->documentMetadata(QStringLiteral("test-widget")));
	controller.widgets()->registerDocumentMetadata(QStringLiteral("Test.Widget"),
		std::make_unique<TestDocumentMetadata>());

	const auto* metadata = controller.widgets()->documentMetadata(QStringLiteral("test-widget"));
	QVERIFY(metadata);
	QCOMPARE(metadata->variables().size(), 4);
	QCOMPARE(metadata->variables().first().name, QStringLiteral("path"));
	QCOMPARE(controller.widgets()->documentMetadata(QStringLiteral("Test.Widget")), metadata);
	QVERIFY(!controller.widgets()->documentMetadata(QStringLiteral("test-singleton")));
}

void TestUiZones::singletoneDocumentKeepsXtypeName() {
	TestCommandController controller;
	int built = 0;
	controller.widgets()->registerFactory({ "Test.Singleton", "test-singleton", /*singletone*/ true,
		QStringLiteral("★"), QStringLiteral("Одиночный") },
		[&built](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			built++;
			return new QLabel(handler.title, handler.parent);
		});
	MainWindow window(controller.globalContext(), nullptr, false);
	WidgetZoneCommand documents(QStringLiteral("documents"), MainWindow::WidgetKind::Document);

	// У команды передан id=test, но документ-синглтон получает имя xtype.
	QVERIFY(documents.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "test-singleton" }, { "id", "test" }
	})));
	QVERIFY(window.widget(QStringLiteral("test-singleton")));
	QVERIFY(!window.widget(QStringLiteral("test")));
	QCOMPARE(built, 1);
	QCOMPARE(window.widgetTitle(QStringLiteral("test-singleton")), QStringLiteral("Одиночный"));
	QCOMPARE(window.document(QStringLiteral("test-singleton"))->glyph(), QStringLiteral("★"));

	// Повторное создание с другим id активирует уже открытый документ.
	QVERIFY(documents.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "test-singleton" }, { "id", "other" }
	})));
	QCOMPARE(built, 1);
	QCOMPARE(window.widgetIds(MainWindow::WidgetKind::Document).size(), 1);
	QCOMPARE(window.activeDocumentId(), QStringLiteral("test-singleton"));
}

void TestUiZones::singletoneCreateWidgetNormalizesId() {
	TestCommandController controller;
	int built = 0;
	controller.widgets()->registerFactory({ "Test.Singleton", "test-singleton", /*singletone*/ true,
		QStringLiteral("★"), QStringLiteral("Одиночный") },
		[&built](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			built++;
			return new QLabel(handler.title, handler.parent);
		});
	MainWindow window(controller.globalContext(), nullptr, false);

	// Имя singletone-документа приводится к xtype уже в createWidget,
	// поэтому фабрика вызывается один раз, а id=test не появляется.
	QVERIFY(window.createWidget(QStringLiteral("test"),
		MainWindow::WidgetKind::Document, QStringLiteral("Документ"), QString(),
		[&built] { return built++, new QLabel; }, QString(), QStringLiteral("test-singleton")));
	QVERIFY(window.widget(QStringLiteral("test-singleton")));
	QVERIFY(!window.widget(QStringLiteral("test")));
	QCOMPARE(built, 1);

	// Повторное создание с другим id активирует уже открытый документ.
	QVERIFY(window.createWidget(QStringLiteral("other"),
		MainWindow::WidgetKind::Document, QStringLiteral("Другой"), QString(),
		[&built] { return built++, new QLabel; }, QString(), QStringLiteral("test-singleton")));
	QCOMPARE(built, 1);
	QCOMPARE(window.widgetIds(MainWindow::WidgetKind::Document).size(), 1);
	QCOMPARE(window.activeDocumentId(), QStringLiteral("test-singleton"));
}

void TestUiZones::commandUsesRegistryDefaults() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);
	WidgetZoneCommand documents(QStringLiteral("documents"), MainWindow::WidgetKind::Document);

	// Без title/glyph в команде берутся значения по умолчанию из регистрации.
	QVERIFY(documents.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "test-widget" }, { "id", "defaults" }
	})));

	auto* document = window.document(QStringLiteral("defaults"));
	QVERIFY(document);
	QCOMPARE(document->title(), QStringLiteral("Тестовый виджет"));
	QCOMPARE(document->glyph(), QStringLiteral("🧪"));
	QCOMPARE(window.widgetType(QStringLiteral("defaults")), QStringLiteral("test-widget"));
}

void TestUiZones::documentStateIsStoredInContext() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);

	QVERIFY(window.createWidget(QStringLiteral("doc"), MainWindow::WidgetKind::Document,
		QStringLiteral("Заголовок"), QStringLiteral("#"),
		[&fixture] { return fixture.built++, new QLabel(QStringLiteral("doc")); },
		QString(), QStringLiteral("test-widget"), controller.globalContext(),
		{ { QStringLiteral("path"), QStringLiteral("notes.txt") },
		  { QStringLiteral("count"), 42 } }));

	auto* document = window.document(QStringLiteral("doc"));
	auto context = window.documentContext(QStringLiteral("doc"));
	QVERIFY(document);
	QVERIFY(context);

	// Состояние документа живёт в переменных его контекста.
	QCOMPARE(context->ownVariables().value(QStringLiteral("title")).toString(),
		QStringLiteral("Заголовок"));
	QCOMPARE(context->ownVariables().value(QStringLiteral("glyph")).toString(),
		QStringLiteral("#"));
	QCOMPARE(context->ownVariables().value(QStringLiteral("path")).toString(),
		QStringLiteral("notes.txt"));
	QCOMPARE(context->ownVariables().value(QStringLiteral("count")).toInt(), 42);

	// Изменение свойства документа обновляет контекст.
	document->setTitle(QStringLiteral("Новый"));
	QCOMPARE(context->variable(QStringLiteral("title")).toString(), QStringLiteral("Новый"));

	// Изменение переменной контекста обновляет документ.
	context->setVariable(QStringLiteral("title"), QStringLiteral("Из контекста"));
	QCOMPARE(document->title(), QStringLiteral("Из контекста"));

	// Значения по умолчанию создаются в контексте при чтении.
	QCOMPARE(document->variable(QStringLiteral("missing"), QStringLiteral("default")).toString(),
		QStringLiteral("default"));
	QCOMPARE(context->ownVariables().value(QStringLiteral("missing")).toString(),
		QStringLiteral("default"));
}

void TestUiZones::commandParametersBecomeDocumentVariables() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	MainWindow window(controller.globalContext(), nullptr, false);
	WidgetZoneCommand documents(QStringLiteral("documents"), MainWindow::WidgetKind::Document);

	// Остальные параметры команды становятся переменными контекста документа.
	QVERIFY(documents.execute(commandContext(controller, {
		{ "action", "create" }, { "target", "test-widget" }, { "id", "vars" },
		{ "path", "hello.txt" }, { "count", 3 }
	})));

	auto context = window.documentContext(QStringLiteral("vars"));
	QVERIFY(context);
	QCOMPARE(context->ownVariables().value(QStringLiteral("path")).toString(),
		QStringLiteral("hello.txt"));
	QCOMPARE(context->ownVariables().value(QStringLiteral("count")).toInt(), 3);
	// Служебные параметры команды в контекст документа не попадают.
	QVERIFY(!context->ownVariables().contains(QStringLiteral("action")));
	QVERIFY(!context->ownVariables().contains(QStringLiteral("target")));
}

void TestUiZones::newDocumentDialogUsesDocumentMetadata() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	controller.widgets()->registerDocumentMetadata(QStringLiteral("Test.Widget"),
		std::make_unique<TestDocumentMetadata>());
	// Начальное значение обязательной переменной берётся из контекста.
	controller.globalContext()->setVariable(QStringLiteral("path"), QStringLiteral("from-context.txt"));

	NewDocumentDialog dialog(controller.widgets(), controller.globalContext());

	auto* list = dialog.findChild<QListWidget*>(QStringLiteral("NewDocumentDialogTypes"));
	auto* properties = dialog.findChild<PropertiesListWidget*>(
		QStringLiteral("NewDocumentDialogVariables"));
	QVERIFY(list);
	QVERIFY(properties);
	QCOMPARE(list->count(), 1);
	list->setCurrentRow(0);

	// Список заполнен обязательными переменными из метаданных типа.
	auto variables = dialog.variables();
	QCOMPARE(variables.size(), 4);
	QCOMPARE(variables.value(QStringLiteral("path")).toString(),
		QStringLiteral("from-context.txt"));
	QCOMPARE(variables.value(QStringLiteral("count")).toInt(), 0);
	QCOMPARE(variables.value(QStringLiteral("enabled")).toBool(), false);
	QCOMPARE(variables.value(QStringLiteral("note")).toString(), QString());

	// Значения, изменённые в редакторах PropertiesListWidget, попадают в результат.
	auto* noteEdit = findPropertyEditor<QLineEdit>(properties, QStringLiteral("note"));
	auto* countEdit = findPropertyEditor<QSpinBox>(properties, QStringLiteral("count"));
	auto* enabledEdit = findPropertyEditor<QCheckBox>(properties, QStringLiteral("enabled"));
	QVERIFY(noteEdit);
	QVERIFY(countEdit);
	QVERIFY(enabledEdit);

	QTest::keyClicks(noteEdit, QStringLiteral("hello"));
	countEdit->setValue(7);
	enabledEdit->setChecked(true);

	variables = dialog.variables();
	QCOMPARE(variables.value(QStringLiteral("note")).toString(), QStringLiteral("hello"));
	QCOMPARE(variables.value(QStringLiteral("count")).toInt(), 7);
	QCOMPARE(variables.value(QStringLiteral("enabled")).toBool(), true);
	QCOMPARE(variables.value(QStringLiteral("path")).toString(),
		QStringLiteral("from-context.txt"));

	auto* addButton = dialog.findChild<QToolButton*>(QStringLiteral("NewDocumentDialogAddVariable"));
	auto* removeButton = dialog.findChild<QToolButton*>(
		QStringLiteral("NewDocumentDialogRemoveVariable"));
	auto* selector = dialog.findChild<QComboBox*>(
		QStringLiteral("NewDocumentDialogVariableSelector"));
	QVERIFY(addButton);
	QVERIFY(removeButton);
	QVERIFY(selector);
	QCOMPARE(selector->count(), 4);

	// Обязательную переменную типа удалить нельзя.
	selector->setCurrentIndex(selector->findData(QStringLiteral("path")));
	QVERIFY(!removeButton->isEnabled());

	// Добавление параметра через диалог: имя и mime-тип.
	const auto addVariable = [&](const QString& name, const QString& mimeType) {
		QTimer::singleShot(0, [name, mimeType] {
			auto* addDialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
			if (!addDialog) {
				return;
			}
			if (auto* nameEdit = addDialog->findChild<QLineEdit*>(
				QStringLiteral("NewDocumentVariableName"))) {
				nameEdit->setText(name);
			}
			if (auto* mimeCombo = addDialog->findChild<QComboBox*>(
				QStringLiteral("NewDocumentVariableMime"))) {
				const int index = mimeCombo->findData(mimeType);
				if (index >= 0) {
					mimeCombo->setCurrentIndex(index);
				}
			}
			addDialog->accept();
		});
		addButton->click();
	};

	addVariable(QStringLiteral("extra"), QString::fromLatin1(DocumentVariableMime::kInteger));
	QVERIFY(dialog.variables().contains(QStringLiteral("extra")));
	QCOMPARE(dialog.variables().value(QStringLiteral("extra")).toInt(), 0);
	QVERIFY(selector->findData(QStringLiteral("extra")) >= 0);

	auto* extraSpin = findPropertyEditor<QSpinBox>(properties, QStringLiteral("extra"));
	QVERIFY(extraSpin);
	extraSpin->setValue(5);
	QCOMPARE(dialog.variables().value(QStringLiteral("extra")).toInt(), 5);

	// Без выбора mime-типа параметр считается обычным текстом.
	addVariable(QStringLiteral("plain"), QString());
	auto* plainEdit = findPropertyEditor<QLineEdit>(properties, QStringLiteral("plain"));
	QVERIFY(plainEdit);
	QTest::keyClicks(plainEdit, QStringLiteral("text"));
	QCOMPARE(dialog.variables().value(QStringLiteral("plain")).toString(),
		QStringLiteral("text"));

	// Удаление добавленного параметра.
	selector->setCurrentIndex(selector->findData(QStringLiteral("extra")));
	QVERIFY(removeButton->isEnabled());
	removeButton->click();
	QVERIFY(!dialog.variables().contains(QStringLiteral("extra")));
	QVERIFY(selector->findData(QStringLiteral("extra")) < 0);
	QVERIFY(!findPropertyEditor<QSpinBox>(properties, QStringLiteral("extra")));
}

void TestUiZones::propertiesListWidgetAddsAndRemoves() {
	PropertiesListWidget widget;

	PropertyData first;
	first.id = QStringLiteral("first");
	first.label = QStringLiteral("Первый");
	first.type = PropertyType::Text;
	first.value = QStringLiteral("one");
	widget.setProperties({ first });

	auto* firstEdit = findPropertyEditor<QLineEdit>(&widget, QStringLiteral("first"));
	QVERIFY(firstEdit);
	QCOMPARE(firstEdit->text(), QStringLiteral("one"));

	PropertyData second;
	second.id = QStringLiteral("second");
	second.label = QStringLiteral("Второй");
	second.type = PropertyType::Boolean;
	second.value = false;
	widget.addProperty(second);

	auto* secondEdit = findPropertyEditor<QCheckBox>(&widget, QStringLiteral("second"));
	QVERIFY(secondEdit);
	QVERIFY(!secondEdit->isChecked());
	secondEdit->setChecked(true);
	QVERIFY(secondEdit->isChecked());

	QVERIFY(widget.removeProperty(QStringLiteral("first")));
	QVERIFY(!findPropertyEditor<QLineEdit>(&widget, QStringLiteral("first")));
	QVERIFY(findPropertyEditor<QCheckBox>(&widget, QStringLiteral("second")));
	QVERIFY(!widget.removeProperty(QStringLiteral("missing")));

	widget.clearProperties();
	QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
	QVERIFY(!findPropertyEditor<QCheckBox>(&widget, QStringLiteral("second")));
}

void TestUiZones::registryBuildWrapsWidgetIntoDocument() {
	TestCommandController controller;
	WidgetFixture fixture(controller);

	WidgetBuildHandler handler;
	auto* document = controller.widgets()->build(QStringLiteral("test-widget"), handler);
	QVERIFY(document);
	QCOMPARE(fixture.built, 1);
	// Фабрика вернула QLabel — build обернул его в документ.
	QVERIFY(qobject_cast<DocumentWidgetStub*>(document));

	// buildWidget возвращает содержимое без обёртки (для вьюпорта).
	auto* raw = controller.widgets()->buildWidget(QStringLiteral("test-widget"), handler);
	QVERIFY(raw);
	QCOMPARE(fixture.built, 2);
	QVERIFY(!qobject_cast<DocumentWidget*>(raw));
	QVERIFY(!qobject_cast<DocumentWidgetStub*>(raw));

	delete raw;
	delete document;
}

void TestUiZones::newDocumentDialogFiltersTypes() {
	TestCommandController controller;
	WidgetFixture fixture(controller);
	controller.widgets()->registerFactory({ "Test.Singleton", "test-singleton", /*singletone*/ true,
		QStringLiteral("★"), QStringLiteral("Одиночный") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new QLabel(handler.title, handler.parent);
		});

	NewDocumentDialog dialog(controller.widgets(), controller.globalContext());

	auto* list = dialog.findChild<QListWidget*>(QStringLiteral("NewDocumentDialogTypes"));
	auto* search = dialog.findChild<QLineEdit*>(QStringLiteral("NewDocumentDialogSearch"));
	auto* name = dialog.findChild<QLineEdit*>(QStringLiteral("NewDocumentDialogName"));
	auto* glyph = dialog.findChild<QLineEdit*>(QStringLiteral("NewDocumentDialogGlyph"));
	auto* title = dialog.findChild<QLineEdit*>(QStringLiteral("NewDocumentDialogTitle"));
	auto* random = dialog.findChild<QToolButton*>(QStringLiteral("NewDocumentDialogRandomName"));
	auto* properties = dialog.findChild<PropertiesListWidget*>(
		QStringLiteral("NewDocumentDialogVariables"));
	QVERIFY(list);
	QVERIFY(search);
	QVERIFY(name);
	QVERIFY(glyph);
	QVERIFY(title);
	QVERIFY(random);
	QVERIFY(properties);

	QCOMPARE(list->count(), 2);
	QCOMPARE(list->item(0)->text(), QStringLiteral("(test-singleton) Test.Singleton"));
	QCOMPARE(list->item(1)->text(), QStringLiteral("(test-widget) Test.Widget"));

	// Тип-синглтон: имя совпадает с xtype и не редактируется.
	list->setCurrentRow(0);
	QVERIFY(dialog.isSingletone());
	QCOMPARE(dialog.documentType(), QStringLiteral("test-singleton"));
	QCOMPARE(dialog.documentName(), QStringLiteral("test-singleton"));
	QVERIFY(!name->isEnabled());
	QCOMPARE(name->text(), QStringLiteral("test-singleton"));
	QCOMPARE(glyph->text(), QStringLiteral("★"));
	QCOMPARE(title->text(), QStringLiteral("Одиночный"));

	// Обычный документ: имя можно изменить и сгенерировать заново.
	list->setCurrentRow(1);
	QVERIFY(!dialog.isSingletone());
	QVERIFY(name->isEnabled());
	QVERIFY(!name->text().isEmpty());
	const auto generated = name->text();
	random->click();
	QVERIFY(name->text() != generated);
	QCOMPARE(glyph->text(), QStringLiteral("🧪"));
	QCOMPARE(title->text(), QStringLiteral("Тестовый виджет"));

	// Без метаданных обязательных переменных нет — список пуст.
	QVERIFY(dialog.variables().isEmpty());

	// Поиск по вхождению в xtype/класс: остаётся только совпавший тип.
	search->setText(QStringLiteral("singleton"));
	QVERIFY(!list->item(0)->isHidden());
	QVERIFY(list->item(1)->isHidden());
	QCOMPARE(dialog.documentType(), QStringLiteral("test-singleton"));

	search->clear();
	QVERIFY(!list->item(0)->isHidden());
	QVERIFY(!list->item(1)->isHidden());
}

QTEST_MAIN(TestUiZones)
#include "test_ui_zones.moc"