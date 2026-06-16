#include "Launcher/main_frame.h"
#include "Launcher/commands/command_console.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/widgets/action_panel/action_panel_login_builder.h"
#include "Launcher/widgets/action_panel/action_panel_by_user_builder.h"
#include "Launcher/widgets/action_panel/action_panel_widget.h"
#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/models/user.h"
#include "Content/BaseWidgets/mdi_area.h"
#include "Libs/Resources/resources.h"

#include <QSplitter>
#include <QTabWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTextEdit>
#include <QStatusBar>
#include <QToolButton>
#include <QTimer>
#include <memory>


class LauncherMainFrame::Private {
public:
	Private(LauncherMainFrame* parent)
	: q(parent) {
		setupMdiArea(parent);
	}

	LauncherMainFrame* q;
	std::unique_ptr<ApplicationController> controller;
	CommandContext* commandContext;
	CommandConsole* commandConsole;
	Resources* resources;
	MdiArea* mdiArea;
	QToolButton* consoleToggleButton;
	ActionPanelWidget* actionPanel = nullptr;

	void setupMdiArea(LauncherMainFrame* parent);
	void setupConsole();
	void setupView();
	void setupActionPanel();
};

LauncherMainFrame::LauncherMainFrame(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->setupConsole();
	d->setupView();

	connect(d->consoleToggleButton, &QToolButton::toggled, this, &LauncherMainFrame::onToggleCommandConsole);

	// Горячая клавиша
	auto toggleAction = new QAction(this);
	toggleAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_QuoteLeft));
	connect(toggleAction, &QAction::triggered, this, [this]() {
		bool newState = !d->commandConsole->isVisible();
		d->consoleToggleButton->setChecked(newState);
		onToggleCommandConsole(newState);
		});

	auto userService = d->commandContext->services()->usersService();
	connect(userService, &UsersService::loggedOut, this, &LauncherMainFrame::onUserLogout);
	connect(userService, &UsersService::loginSuccess, this, &LauncherMainFrame::onUserLogin);

	addAction(toggleAction);

	d->controller->executeCommandByName("window-create", QStringList{ "target:warmup", "id:opengl-warmup" });
}

LauncherMainFrame::~LauncherMainFrame() = default;

void LauncherMainFrame::Private::setupMdiArea(LauncherMainFrame* parent) {
	mdiArea = new MdiArea(parent);
	mdiArea->setTabsMovable(true);
	mdiArea->setActivationOrder(QMdiArea::ActivationHistoryOrder);
	mdiArea->setDocumentMode(true);
	mdiArea->setOption(QMdiArea::DontMaximizeSubWindowOnActivation);
	mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	mdiArea->setTabPosition(QTabWidget::North);
	mdiArea->setTabsClosable(true);
	mdiArea->setAcceptDrops(true);
	mdiArea->setViewMode(QMdiArea::SubWindowView);
}

void LauncherMainFrame::Private::setupConsole() {
	controller = std::make_unique<ApplicationController>(resources);
	commandContext = controller->commandContext();
	commandConsole = new CommandConsole(controller.get(), commandContext, q);
	commandConsole->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
	q->onToggleCommandConsole(false);

	commandContext->controllers()->windowsController()->setMdiArea(mdiArea);

	// Кнопка переключения в статусбаре
	consoleToggleButton = new QToolButton(q->statusBar());
	consoleToggleButton->setIcon(QIcon::fromTheme("utilities-terminal", QIcon(":/icons/terminal.png")));
	consoleToggleButton->setToolTip("Command Console (Ctrl+`)");
	consoleToggleButton->setCheckable(true);
	consoleToggleButton->setAutoRaise(true);
	consoleToggleButton->setChecked(false);

	q->statusBar()->addPermanentWidget(consoleToggleButton);
}

void LauncherMainFrame::Private::setupView() {
	// Горизонтальный сплиттер для основной области и панели действий
	auto* horizontalSplitter = new QSplitter(Qt::Horizontal);

	// Вертикальный сплиттер для MDI и консоли
	auto* verticalSplitter = new QSplitter(Qt::Vertical);
	verticalSplitter->addWidget(mdiArea);
	verticalSplitter->addWidget(commandConsole);
	verticalSplitter->setStretchFactor(0, 1);
	verticalSplitter->setStretchFactor(1, 0);

	horizontalSplitter->addWidget(verticalSplitter);

	// Панель действий
	actionPanel = new ActionPanelWidget(commandContext->controllers()->actionPanelController(), q);
	horizontalSplitter->addWidget(actionPanel);

	horizontalSplitter->setStretchFactor(0, 1);
	horizontalSplitter->setStretchFactor(1, 0);

	// Фиксируем размер сплиттера (запрещаем изменение)
	horizontalSplitter->setHandleWidth(0);

	q->setCentralWidget(horizontalSplitter);

	// Добавляем кнопку Login
	setupActionPanel();
}

void LauncherMainFrame::Private::setupActionPanel() {
	ActionPanelLoginBuilder builder(commandContext->controllers()->actionPanelController());
	builder.build();
}



void LauncherMainFrame::onToggleCommandConsole(bool visible) {
	if (visible) {
		d->commandConsole->showConsole();
	}
	else {
		d->commandConsole->hideConsole();
	}
}

void LauncherMainFrame::onUserLogout() {
	d->setupActionPanel();
}

void LauncherMainFrame::onUserLogin(const UserData& user) {
	qDebug() << "User login" << user.displayName;
	ActionPanelByUserBuilder builder(
		d->commandContext->controllers()->actionPanelController(),
		d->commandContext->services()->usersService());
	builder.build();
}
