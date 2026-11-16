#include "Libs/Engine/scripting/application/class_manager.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/doc_tab_panel.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Resources/directories/directories_context.h"
#include "Libs/Resources/resources.h"

#include <QDir>
#include <QFile>
#include <QLabel>
#include <QTemporaryDir>
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

/// Одностраничное приложение: один документ, без панелей и без панели вкладок.
constexpr const char* kSinglePageDescription = R"(define:
  name: TestViewportForm
  extends: System.Viewport.MainForm
  xtype: TestViewport
  title: Test

  content:
    - documents:
        showTabs: false
        content:
          - xtype: test-document
            name: main
            glyph: "#"
            title: Главная

    - bottombar:
        delete: true

    - sidebar:
        delete: true
)";

/// Только удалённые панели, без документов: значения delete по умолчанию не заданы.
constexpr const char* kPanelsKeptDescription = R"(define:
  name: TestViewportForm
  extends: System.Viewport.MainForm
  xtype: TestViewport
  title: Test

  content:
    - bottombar:
        default: console
        content:
          - xtype: test-panel
            name: console
            title: Консоль

    - sidebar:
        default: explorer
        content:
          - xtype: test-panel
            name: explorer
            title: Проводник
)";

QStringList capturedWarnings;
QtMessageHandler previousHandler = nullptr;

void collectWarnings(QtMsgType type, const QMessageLogContext& context, const QString& message) {
	if (type == QtWarningMsg) {
		capturedWarnings << message;
	}

	if (previousHandler) {
		previousHandler(type, context, message);
	}
}

} // namespace

class TestClassManager : public QObject {
	Q_OBJECT

private slots:
	void init();
	void cleanup();

	void singlePageOpensDocumentWithoutZones();
	void deletedZoneRejectsPanelWithWarning();
	void panelsAreBuiltWhenNotDeleted();
	void shippedSinglePageDescriptionLoads();

private:
	/// Готовит профиль с описанием applications/{name}/main.yaml.
	static std::unique_ptr<QTemporaryDir> makeProfile(const QString& application,
		const char* description, QDir* root);
};

void TestClassManager::init() {
	capturedWarnings.clear();
	previousHandler = qInstallMessageHandler(collectWarnings);
}

void TestClassManager::cleanup() {
	qInstallMessageHandler(previousHandler);
	previousHandler = nullptr;
}

std::unique_ptr<QTemporaryDir> TestClassManager::makeProfile(const QString& application,
	const char* description, QDir* root) {
	auto directory = std::make_unique<QTemporaryDir>();
	if (!directory->isValid()) {
		return nullptr;
	}

	root->setPath(directory->path());

	const auto path = QDir(directory->path()).filePath("applications/" + application);
	if (!QDir().mkpath(path)) {
		return nullptr;
	}

	QFile file(path + QStringLiteral("/main.yaml"));
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
		return nullptr;
	}

	file.write(description);
	file.close();
	return directory;
}

void TestClassManager::singlePageOpensDocumentWithoutZones() {
	QDir root;
	auto directory = makeProfile(QStringLiteral("test"), kSinglePageDescription, &root);
	QVERIFY(directory);

	Resources resources;
	resources.Directories.set(QStringLiteral("path.profile"), root);

	TestCommandController controller;
	controller.widgets()->registerFactory({ "System.Viewport.MainForm", "mainform" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent, /*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});
	controller.widgets()->registerFactory({ "Test.Document", "test-document" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			auto* document = new DocumentWidget(handler.parent);
			document->setTitle(handler.title);
			return document;
		});

	ClassManager manager(&resources, &controller);
	BootstrapConfig::Application application;
	application.name = QStringLiteral("test");
	application.viewport = QStringLiteral("TestViewport");

	auto* viewport = manager.createViewport(application);
	QVERIFY(viewport);

	auto* window = qobject_cast<MainWindow*>(viewport);
	QVERIFY(window);

	// Документ из описания открыт и зарегистрирован как документ.
	QCOMPARE(window->documentArea()->documents().size(), 1);
	QVERIFY(window->widget(QStringLiteral("main")));
	QCOMPARE(window->widgetKind(QStringLiteral("main")),
		std::optional<MainWindow::WidgetKind>(MainWindow::WidgetKind::Document));
	QCOMPARE(window->widgetTitle(QStringLiteral("main")), QStringLiteral("Главная"));

	// Панель открытых документов скрыта (documents.showTabs: false).
	QVERIFY(!window->isDocumentTabsVisible());
	auto* tabs = window->findChild<DocumentTabsPanel*>();
	QVERIFY(tabs);
	QVERIFY(tabs->isHidden());

	// Панели удалены описанием.
	QVERIFY(!window->isZoneEnabled(MainWindow::WidgetKind::BottomPanel));
	QVERIFY(!window->isZoneEnabled(MainWindow::WidgetKind::SidePanel));
	QVERIFY(window->widgetIds(MainWindow::WidgetKind::BottomPanel).isEmpty());
	QVERIFY(window->widgetIds(MainWindow::WidgetKind::SidePanel).isEmpty());

	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::deletedZoneRejectsPanelWithWarning() {
	TestCommandController controller;
	MainWindow window(controller.globalContext(), nullptr, /*buildDefaultPanels*/ false,
		MainWindow::Options::fromBuildOptions({ { QStringLiteral("bottombar.delete"), true },
			{ QStringLiteral("sidebar.delete"), true } }));
	QVERIFY(!window.isZoneEnabled(MainWindow::WidgetKind::BottomPanel));
	QVERIFY(!window.isZoneEnabled(MainWindow::WidgetKind::SidePanel));

	capturedWarnings.clear();
	auto* panel = new QLabel;
	QVERIFY(!window.createWidget(QStringLiteral("info"), MainWindow::WidgetKind::BottomPanel,
		QStringLiteral("Вывод"), QString(), [panel] { return panel; }));
	QVERIFY(capturedWarnings.join(QLatin1Char('\n')).contains(QStringLiteral("bottombar is deleted")));

	capturedWarnings.clear();
	auto* sidePanel = new QLabel;
	QVERIFY(!window.createWidget(QStringLiteral("side"), MainWindow::WidgetKind::SidePanel,
		QStringLiteral("Свойства"), QString(), [sidePanel] { return sidePanel; }));
	QVERIFY(capturedWarnings.join(QLatin1Char('\n')).contains(QStringLiteral("sidebar is deleted")));

	// Виджеты не приняты и не зарегистрированы окном.
	QVERIFY(window.widgetIds().isEmpty());
	delete panel;
	delete sidePanel;
}

void TestClassManager::panelsAreBuiltWhenNotDeleted() {
	QDir root;
	auto directory = makeProfile(QStringLiteral("test"), kPanelsKeptDescription, &root);
	QVERIFY(directory);

	Resources resources;
	resources.Directories.set(QStringLiteral("path.profile"), root);

	TestCommandController controller;
	controller.widgets()->registerFactory({ "System.Viewport.MainForm", "mainform" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent, /*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});
	controller.widgets()->registerFactory({ "Test.Panel", "test-panel" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new QLabel(handler.title, handler.parent);
		});

	ClassManager manager(&resources, &controller);
	BootstrapConfig::Application application;
	application.name = QStringLiteral("test");
	application.viewport = QStringLiteral("TestViewport");

	auto* viewport = manager.createViewport(application);
	QVERIFY(viewport);

	auto* window = qobject_cast<MainWindow*>(viewport);
	QVERIFY(window);

	// Без delete зоны создаются и принимают панели из описания.
	QVERIFY(window->isZoneEnabled(MainWindow::WidgetKind::BottomPanel));
	QVERIFY(window->isZoneEnabled(MainWindow::WidgetKind::SidePanel));
	QCOMPARE(window->widgetIds(MainWindow::WidgetKind::BottomPanel),
		QStringList{ QStringLiteral("console") });
	QCOMPARE(window->widgetIds(MainWindow::WidgetKind::SidePanel),
		QStringList{ QStringLiteral("explorer") });
	QCOMPARE(window->widgetType(QStringLiteral("console")), QStringLiteral("test-panel"));

	// showTabs не задан — панель открытых документов остаётся включённой.
	QVERIFY(window->isDocumentTabsVisible());

	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::shippedSinglePageDescriptionLoads() {
#ifdef REDCOMPLEX_CONFIGURATIONS_DIR
	QDir root(QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR));
#else
	QDir root;
#endif
	if (!root.exists(QStringLiteral("applications/singletone/main.yaml"))) {
		QSKIP("Эталонные описания приложений недоступны");
	}

	Resources resources;
	resources.Directories.set(QStringLiteral("path.profile"), root);

	TestCommandController controller;
	controller.widgets()->registerFactory({ "System.Viewport.MainForm", "mainform" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent, /*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});
	controller.widgets()->registerFactory({ "System.Panels.ConsolePanel", "console" },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new QLabel(handler.title, handler.parent);
		});

	ClassManager manager(&resources, &controller);
	BootstrapConfig::Application application;
	application.name = QStringLiteral("singletone");
	application.viewport = QStringLiteral("SingletoneViewport");

	auto* viewport = manager.createViewport(application);
	QVERIFY(viewport);

	auto* window = qobject_cast<MainWindow*>(viewport);
	QVERIFY(window);

	// Одностраничный документ: одна консоль, без вкладок и без боковых панелей.
	QCOMPARE(window->documentArea()->documents().size(), 1);
	QCOMPARE(window->widgetType(QStringLiteral("console")), QStringLiteral("console"));
	QVERIFY(!window->isDocumentTabsVisible());
	QVERIFY(!window->isZoneEnabled(MainWindow::WidgetKind::BottomPanel));
	QVERIFY(!window->isZoneEnabled(MainWindow::WidgetKind::SidePanel));

	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

QTEST_MAIN(TestClassManager)
#include "test_class_manager.moc"
