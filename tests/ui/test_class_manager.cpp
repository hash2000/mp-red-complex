#include "Libs/Engine/scripting/application/class_manager.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/widgets/widget_registry.h"
#include "Libs/Engine/ui/activity_bar.h"
#include "Libs/Engine/ui/bottom_panel.h"
#include "Libs/Engine/ui/doc_area.h"
#include "Libs/Engine/ui/doc_tab_panel.h"
#include "Libs/Engine/ui/doc_widget.h"
#include "Libs/Engine/ui/main_window.h"
#include "Libs/Engine/ui/side_host_panel.h"
#include "Libs/Resources/directories/directories_context.h"
#include "Libs/Resources/resources.h"

#include <QAction>
#include <QApplication>
#include <QDir>
#include <QEnterEvent>
#include <QFile>
#include <QImage>
#include <QLabel>
#include <QSplitter>
#include <QTabBar>
#include <QTemporaryDir>
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

/// Все флаги зон: нижняя свёрнута и без кнопок, боковая развёрнута,
/// потому что collapsable: false важнее collapsed.
constexpr const char* kZoneOptionsDescription = R"(define:
  name: TestViewportForm
  extends: System.Viewport.MainForm
  xtype: TestViewport
  title: Test

  content:
    - bottombar:
        collapsable: false
        closable: false
        autohide: true
        default: console
        content:
          - xtype: test-panel
            name: console
            title: Консоль

    - sidebar:
        collapsed: true
        collapsable: false
        autohide: true
        default: explorer
        content:
          - xtype: test-panel
            name: explorer
            title: Проводник
)";

/// Боковая панель скрыта, панель кнопок при этом работает как обычно.
constexpr const char* kSidebarCollapsedDescription = R"(define:
  name: TestViewportForm
  extends: System.Viewport.MainForm
  xtype: TestViewport
  title: Test

  content:
    - bottombar:
        delete: true

    - sidebar:
        collapsed: true
        default: explorer
        content:
          - xtype: test-panel
            name: explorer
            title: Проводник
)";

/// Панель кнопок свёрнута после загрузки вместе с боковыми панелями.
constexpr const char* kSidebarAutoHideDescription = R"(define:
  name: TestViewportForm
  extends: System.Viewport.MainForm
  xtype: TestViewport
  title: Test

  content:
    - bottombar:
        delete: true

    - sidebar:
        autohide: true
        default: explorer
        content:
          - xtype: test-panel
            name: explorer
            title: Проводник
)";

/// Нижняя панель свёрнута: шапки нет, но полоска вкладок со всеми панелями видна.
constexpr const char* kBottomCollapsedDescription = R"(define:
  name: TestViewportForm
  extends: System.Viewport.MainForm
  xtype: TestViewport
  title: Test

  content:
    - bottombar:
        collapsed: true
        default: console
        content:
          - xtype: test-panel
            name: console
            title: Консоль

          - xtype: test-panel
            name: errors
            title: Ошибки

          - xtype: test-panel
            name: output
            title: Вывод

    - sidebar:
        delete: true
)";

/// Нижняя панель автоскрыта в тонкую горизонтальную полоску.
constexpr const char* kBottomAutoHideDescription = R"(define:
  name: TestViewportForm
  extends: System.Viewport.MainForm
  xtype: TestViewport
  title: Test

  content:
    - bottombar:
        autohide: true
        default: console
        content:
          - xtype: test-panel
            name: console
            title: Консоль

    - sidebar:
        delete: true
)";

QStringList capturedWarnings;
QtMessageHandler previousHandler = nullptr;

QAction* findActionWithText(QWidget* widget, const QString& text) {
	for (auto* action : widget->findChildren<QAction*>()) {
		if (action->text().contains(text)) {
			return action;
		}
	}

	return nullptr;
}

QToolButton* findButtonByToolTip(QWidget* widget, const QString& toolTip) {
	for (auto* button : widget->findChildren<QToolButton*>()) {
		if (button->toolTip() == toolTip) {
			return button;
		}
	}

	return nullptr;
}

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
	void zoneOptionsDefaultsKeepCurrentUi();
	void zoneOptionsAreApplied();
	void collapsedBottomKeepsTabStrip();
	void bottomAutoHideFromDescription();
	void sidebarCollapsedHidesPanels();
	void sidebarAutoHideFromDescription();
	void autoHideHidesSidePanels();
	void activityBarPinGlyphIsVisible();
	void shippedSinglePageDescriptionLoads();

private:
	/// Готовит профиль с описанием applications/{name}/main.yaml.
	static std::unique_ptr<QTemporaryDir> makeProfile(const QString& application,
		const char* description, QDir* root);
	/// Создаёт MainWindow по описанию с тестовыми типами виджетов.
	static MainWindow* createTestWindow(const char* description,
		std::unique_ptr<QTemporaryDir>& directory, Resources& resources,
		TestCommandController& controller);
};

void TestClassManager::init() {
	capturedWarnings.clear();
	previousHandler = qInstallMessageHandler(collectWarnings);
}

void TestClassManager::cleanup() {
	qInstallMessageHandler(previousHandler);
	previousHandler = nullptr;
	qApp->setStyleSheet(QString());
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

MainWindow* TestClassManager::createTestWindow(const char* description,
	std::unique_ptr<QTemporaryDir>& directory, Resources& resources,
	TestCommandController& controller) {
	QDir root;
	directory = makeProfile(QStringLiteral("test"), description, &root);
	if (!directory) {
		return nullptr;
	}

	resources.Directories.set(QStringLiteral("path.profile"), root);

	controller.widgets()->registerFactory({ "System.Viewport.MainForm", "mainform",
		/*singletone*/ true, QStringLiteral("Оболочка") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent, /*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});
	controller.widgets()->registerFactory({ "Test.Panel", "test-panel", /*singletone*/ false },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new QLabel(handler.title, handler.parent);
		});

	ClassManager manager(&resources, &controller);
	BootstrapConfig::Application application;
	application.name = QStringLiteral("test");
	application.viewport = QStringLiteral("TestViewport");

	return qobject_cast<MainWindow*>(manager.createViewport(application));
}

void TestClassManager::singlePageOpensDocumentWithoutZones() {
	QDir root;
	auto directory = makeProfile(QStringLiteral("test"), kSinglePageDescription, &root);
	QVERIFY(directory);

	Resources resources;
	resources.Directories.set(QStringLiteral("path.profile"), root);

	TestCommandController controller;
	controller.widgets()->registerFactory({ "System.Viewport.MainForm", "mainform",
		/*singletone*/ true, QStringLiteral("Оболочка") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent, /*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});
	controller.widgets()->registerFactory({ "Test.Document", "test-document",
		/*singletone*/ false },
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
	controller.widgets()->registerFactory({ "System.Viewport.MainForm", "mainform",
		/*singletone*/ true, QStringLiteral("Оболочка") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent, /*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});
	controller.widgets()->registerFactory({ "Test.Panel", "test-panel", /*singletone*/ false },
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

void TestClassManager::zoneOptionsDefaultsKeepCurrentUi() {
	std::unique_ptr<QTemporaryDir> directory;
	Resources resources;
	TestCommandController controller;

	auto* window = createTestWindow(kPanelsKeptDescription, directory, resources, controller);
	QVERIFY(window);

	// Без параметров всё как раньше: панели развёрнуты и видны.
	auto* bottom = window->findChild<BottomPanel*>();
	QVERIFY(bottom);
	QVERIFY(!bottom->isCollapsed());
	QVERIFY(bottom->isCollapsable());
	QVERIFY(bottom->isClosable());
	QVERIFY(!bottom->autoHide());

	auto* pinButton = bottom->findChild<QToolButton*>(QStringLiteral("bottomPinButton"));
	QVERIFY(pinButton);
	QVERIFY(!pinButton->isHidden());
	QVERIFY(!findButtonByToolTip(bottom, QStringLiteral("Свернуть")));
	QVERIFY(!findButtonByToolTip(bottom, QStringLiteral("Скрыть (Ctrl+J)")));

	auto* side = window->findChild<SideHostPanel*>();
	QVERIFY(side);
	QVERIFY(!side->isHidden());

	auto* activity = window->findChild<ActivityBar*>();
	QVERIFY(activity);
	QVERIFY(activity->isPinVisible());
	QVERIFY(!activity->autoHide());

	QVERIFY(findActionWithText(window, QStringLiteral("Нижняя панель")));
	QVERIFY(findActionWithText(window, QStringLiteral("Закрепить панель кнопок")));

	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::zoneOptionsAreApplied() {
	std::unique_ptr<QTemporaryDir> directory;
	Resources resources;
	TestCommandController controller;

	auto* window = createTestWindow(kZoneOptionsDescription, directory, resources, controller);
	QVERIFY(window);

	// bottombar: свёрнута, кнопок сворачивания и закрытия нет, меню нет.
	auto* bottom = window->findChild<BottomPanel*>();
	QVERIFY(bottom);
	QVERIFY(bottom->isCollapsed());
	QVERIFY(!bottom->isCollapsable());
	QVERIFY(!bottom->isClosable());
	QVERIFY(!bottom->autoHide());
	QVERIFY(bottom->collapsedHeight() > 0);

	auto* pinButton = bottom->findChild<QToolButton*>(QStringLiteral("bottomPinButton"));
	QVERIFY(pinButton);
	QVERIFY(pinButton->isHidden());
	QVERIFY(!findButtonByToolTip(bottom, QStringLiteral("Свернуть")));
	QVERIFY(!findButtonByToolTip(bottom, QStringLiteral("Скрыть (Ctrl+J)")));
	QVERIFY(!findActionWithText(window, QStringLiteral("Нижняя панель")));

	// sidebar: collapsable: false важнее collapsed — панель развёрнута,
	// кнопки сворачивания нет.
	auto* side = window->findChild<SideHostPanel*>();
	QVERIFY(side);
	QVERIFY(!side->isHidden());

	auto* activity = window->findChild<ActivityBar*>();
	QVERIFY(activity);
	QVERIFY(!activity->isPinVisible());
	QVERIFY(!activity->autoHide());
	QVERIFY(!findActionWithText(window, QStringLiteral("Закрепить панель кнопок")));

	// Свёрнутая нижняя панель имеет видимую высоту полоски вкладок.
	window->resize(1200, 800);
	window->show();
	QTest::qWait(50);
	QVERIFY(bottom->isVisible());
	QVERIFY2(bottom->height() >= 10, qPrintable(QString::number(bottom->height())));
	window->hide();

	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::collapsedBottomKeepsTabStrip() {
	std::unique_ptr<QTemporaryDir> directory;
	Resources resources;
	TestCommandController controller;

	auto* window = createTestWindow(kBottomCollapsedDescription, directory, resources, controller);
	QVERIFY(window);

	// Шапка скрыта, но полоска вкладок со всеми панелями остаётся.
	auto* bottom = window->findChild<BottomPanel*>();
	QVERIFY(bottom);
	QVERIFY(bottom->isCollapsed());
	QVERIFY(bottom->isCollapsable());
	QVERIFY(bottom->isClosable());
	QVERIFY(bottom->collapsedHeight() > 0);

	auto* tabs = bottom->findChild<QTabBar*>(QStringLiteral("BottomTabs"));
	QVERIFY(tabs);
	QCOMPARE(tabs->count(), 3);

	auto* pinButton = bottom->findChild<QToolButton*>(QStringLiteral("bottomPinButton"));
	QVERIFY(pinButton);
	QVERIFY(!bottom->autoHide());
	QVERIFY(findActionWithText(window, QStringLiteral("Нижняя панель")));

	// Старых кнопок шапки больше нет.
	QVERIFY(!findButtonByToolTip(bottom, QStringLiteral("Свернуть")));
	QVERIFY(!findButtonByToolTip(bottom, QStringLiteral("Скрыть (Ctrl+J)")));

	window->resize(1200, 800);
	window->show();
	QTest::qWait(50);
	QVERIFY2(bottom->height() >= 10, qPrintable(QString::number(bottom->height())));
	QVERIFY2(bottom->height() <= bottom->collapsedHeight() + 1,
		qPrintable(QStringLiteral("%1 > %2").arg(bottom->height()).arg(bottom->collapsedHeight())));

	// QSplitter тоже должен пересчитать размер, а не оставлять старые 220 px.
	auto* splitter = qobject_cast<QSplitter*>(bottom->parentWidget());
	QVERIFY(splitter);
	const auto splitterSizes = splitter->sizes();
	QCOMPARE(splitterSizes.size(), 2);
	QVERIFY2(splitterSizes.value(1) <= bottom->collapsedHeight() + 1,
		qPrintable(QStringLiteral("%1 > %2").arg(splitterSizes.value(1)).arg(bottom->collapsedHeight())));

	QVERIFY(tabs->isVisible());
	QVERIFY(pinButton->isVisible());
	QCOMPARE(pinButton->parentWidget(), tabs->parentWidget());
	QVERIFY(pinButton->x() < tabs->x());
	QVERIFY(pinButton->y() <= tabs->y());
	window->hide();

	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::bottomAutoHideFromDescription() {
	std::unique_ptr<QTemporaryDir> directory;
	Resources resources;
	TestCommandController controller;

	auto* window = createTestWindow(kBottomAutoHideDescription, directory, resources, controller);
	QVERIFY(window);

	auto* bottom = window->findChild<BottomPanel*>();
	QVERIFY(bottom);

	// autohide: true — содержимое свёрнуто, видна полоска вкладок.
	QVERIFY(bottom->autoHide());
	QVERIFY(bottom->isCollapsed());
	QVERIFY(bottom->collapsedHeight() > 0);

	auto* pinButton = bottom->findChild<QToolButton*>(QStringLiteral("bottomPinButton"));
	QVERIFY(pinButton);

	window->resize(1200, 800);
	window->show();
	QTest::qWait(50);
	QVERIFY(bottom->isVisible());
	QVERIFY2(bottom->height() <= bottom->collapsedHeight() + 1,
		qPrintable(QString::number(bottom->height())));
	QVERIFY(pinButton->isVisible());

	// Наведение раскрывает содержимое панели.
	const QPointF pos(1, 1);
	QEnterEvent enter(pos, pos, pos);
	QApplication::sendEvent(bottom, &enter);
	QTest::qWait(50);
	QVERIFY(!bottom->isCollapsed());
	QVERIFY2(bottom->height() > bottom->collapsedHeight(),
		qPrintable(QString::number(bottom->height())));

	// Уход мыши сворачивает её обратно по таймеру.
	QEvent leave(QEvent::Leave);
	QApplication::sendEvent(bottom, &leave);
	QTest::qWait(500);
	QVERIFY(bottom->isCollapsed());
	QVERIFY2(bottom->height() <= bottom->collapsedHeight() + 1,
		qPrintable(QString::number(bottom->height())));

	// Кнопка в полоске вкладок снимает автоскрытие и раскрывает панель.
	pinButton->click();
	QVERIFY(!bottom->autoHide());
	QVERIFY(!bottom->isCollapsed());

	// И снова включает автоскрытие.
	pinButton->click();
	QVERIFY(bottom->autoHide());
	QVERIFY(bottom->isCollapsed());

	bottom->setAutoHide(false);
	QVERIFY(!bottom->isCollapsed());

	window->hide();
	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::sidebarCollapsedHidesPanels() {
	std::unique_ptr<QTemporaryDir> directory;
	Resources resources;
	TestCommandController controller;

	auto* window = createTestWindow(kSidebarCollapsedDescription, directory, resources, controller);
	QVERIFY(window);

	// Панели скрыты, панель кнопок работает как обычно.
	auto* side = window->findChild<SideHostPanel*>();
	QVERIFY(side);
	QVERIFY(side->isHidden());

	auto* activity = window->findChild<ActivityBar*>();
	QVERIFY(activity);
	QVERIFY(activity->isPinVisible());
	QVERIFY(!activity->autoHide());
	QVERIFY(findActionWithText(window, QStringLiteral("Закрепить панель кнопок")));

	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::sidebarAutoHideFromDescription() {
	std::unique_ptr<QTemporaryDir> directory;
	Resources resources;
	TestCommandController controller;

	auto* window = createTestWindow(kSidebarAutoHideDescription, directory, resources, controller);
	QVERIFY(window);

	auto* side = window->findChild<SideHostPanel*>();
	QVERIFY(side);
	auto* activity = window->findChild<ActivityBar*>();
	QVERIFY(activity);

	// autohide: true — панель кнопок свёрнута, боковая панель скрыта.
	QVERIFY(activity->autoHide());
	QVERIFY(activity->isPinVisible());
	QVERIFY(side->isHidden());

	window->resize(1200, 800);
	window->show();
	QTest::qWait(50);
	QVERIFY(side->isHidden());

	// Разворачивание возвращает панель, которая была видна до autohide.
	activity->setAutoHide(false);
	QVERIFY(!side->isHidden());
	window->hide();

	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::autoHideHidesSidePanels() {
	std::unique_ptr<QTemporaryDir> directory;
	Resources resources;
	TestCommandController controller;

	auto* window = createTestWindow(kPanelsKeptDescription, directory, resources, controller);
	QVERIFY(window);

	auto* side = window->findChild<SideHostPanel*>();
	QVERIFY(side);
	auto* activity = window->findChild<ActivityBar*>();
	QVERIFY(activity);

	window->resize(1200, 800);
	window->show();
	QTest::qWait(50);
	QVERIFY(!side->isHidden());

	// Autohide сворачивает панель кнопок — вместе с ней скрывается панель.
	activity->setAutoHide(true);
	QVERIFY(activity->autoHide());
	QVERIFY(side->isHidden());

	// Разворачивание возвращает панель, которая была видна до сворачивания.
	activity->setAutoHide(false);
	QVERIFY(!side->isHidden());

	// Если панель была скрыта заранее, разворачивание её не открывает.
	window->hideSidePanel();
	QVERIFY(side->isHidden());
	activity->setAutoHide(true);
	activity->setAutoHide(false);
	QVERIFY(side->isHidden());

	window->hide();
	QVERIFY2(capturedWarnings.isEmpty(), qPrintable(capturedWarnings.join(QLatin1Char('\n'))));
}

void TestClassManager::activityBarPinGlyphIsVisible() {
#ifdef REDCOMPLEX_CONFIGURATIONS_DIR
	QFile stylesheet(QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR)
		+ QStringLiteral("/assets/themes/dark/stylesheets/base.qss"));
	QVERIFY2(stylesheet.open(QIODevice::ReadOnly | QIODevice::Text),
		qPrintable(stylesheet.fileName()));
	qApp->setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
#else
	QSKIP("Эталонная тема недоступна");
#endif

	ActivityBar bar;
	bar.addButton(QStringLiteral("test"), QStringLiteral("#"), QStringLiteral("test"));
	bar.resize(48, 400);
	bar.show();
	QTest::qWait(50);

	auto* pin = bar.findChild<QToolButton*>(QStringLiteral("pinButton"));
	QVERIFY(pin);
	const QImage image = pin->grab().toImage();
	QVERIFY(!image.isNull());

	// Стрелка рисуется цветом темы (#d4d4d4), фон — тёмный/прозрачный.
	int brightPixels = 0;
	for (int y = 0; y < image.height(); y++) {
		for (int x = 0; x < image.width(); x++) {
			if (image.pixelColor(x, y).lightness() > 120) {
				brightPixels++;
			}
		}
	}

	QVERIFY2(brightPixels > 0, "стрелка pinButton не отрисовалась");
	bar.hide();

	// Такая же маленькая кнопка автоскрытия в полоске вкладок BottomPanel.
	BottomPanel bottom;
	bottom.addPanel(QStringLiteral("test"), QStringLiteral("#"), QStringLiteral("test"),
		new QLabel(QStringLiteral("test")));
	bottom.resize(600, 300);
	bottom.show();
	QTest::qWait(50);

	auto* bottomPin = bottom.findChild<QToolButton*>(QStringLiteral("bottomPinButton"));
	QVERIFY(bottomPin);
	const QImage bottomImage = bottomPin->grab().toImage();
	QVERIFY(!bottomImage.isNull());

	int bottomBrightPixels = 0;
	for (int y = 0; y < bottomImage.height(); y++) {
		for (int x = 0; x < bottomImage.width(); x++) {
			if (bottomImage.pixelColor(x, y).lightness() > 120) {
				bottomBrightPixels++;
			}
		}
	}

	QVERIFY2(bottomBrightPixels > 0, "стрелка bottomPinButton не отрисовалась");
	bottom.hide();
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
	controller.widgets()->registerFactory({ "System.Viewport.MainForm", "mainform",
		/*singletone*/ true, QStringLiteral("Оболочка") },
		[](const QString&, const WidgetBuildHandler& handler) -> QWidget* {
			return new MainWindow(handler.context, handler.parent, /*buildDefaultPanels*/ false,
				MainWindow::Options::fromBuildOptions(handler.options));
		});
	controller.widgets()->registerFactory({ "System.Panels.ConsolePanel", "console",
		/*singletone*/ false },
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
