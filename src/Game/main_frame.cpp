#include "Game/main_frame.h"
#include "Game/commands/command_console.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/controllers.h"
#include "Game/controllers/windows_controller.h"
#include "Game/controllers/action_panel_controller.h"
#include "Game/controllers/action_button_config.h"
#include "Game/widgets/action_panel/action_panel_widget.h"
#include "BaseWidgets/mdi_area.h"
#include "Resources/resources.h"

#include <QSplitter>
#include <QTabWidget>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTextEdit>
#include <QStatusBar>
#include <QToolButton>
#include <memory>


class GameMainFrame::Private {
public:
	Private(GameMainFrame* parent)
	: q(parent) {
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

	void setupConsole() {
		controller = new ApplicationController(resources);
		commandConsole = new CommandConsole(controller, q);
		commandConsole->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);
		q->toggleCommandConsole(false);

		controller->controllers()->windowsController()->setMdiArea(mdiArea);

		// Кнопка переключения в статусбаре
		consoleToggleButton = new QToolButton(q->statusBar());
		consoleToggleButton->setIcon(QIcon::fromTheme("utilities-terminal", QIcon(":/icons/terminal.png")));
		consoleToggleButton->setToolTip("Command Console (Ctrl+`)");
		consoleToggleButton->setCheckable(true);
		consoleToggleButton->setAutoRaise(true);
		consoleToggleButton->setChecked(false);

		q->statusBar()->addPermanentWidget(consoleToggleButton);
	}

	void setupView() {
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
		actionPanel = new ActionPanelWidget(
			controller->controllers()->actionPanelController(),
			controller->services()->texturesService(),
			q);
		horizontalSplitter->addWidget(actionPanel);

		horizontalSplitter->setStretchFactor(0, 1);
		horizontalSplitter->setStretchFactor(1, 0);

		// Фиксируем размер сплиттера (запрещаем изменение)
		horizontalSplitter->setHandleWidth(0);

		q->setCentralWidget(horizontalSplitter);

		// Добавляем кнопку Login
		setupActionPanel();
	}

	void setupActionPanel() {
		// Кнопка Login
		ActionButtonConfig loginButton(
			"login",
			"login.png",  // Имя файла иконки (путь будет icons/login.png)
			"Войти в систему",
			"window-create login",
			1
		);
		controller->controllers()->actionPanelController()->addButton(loginButton);
	}

	GameMainFrame* q;
	Resources* resources;
	MdiArea* mdiArea;
	ApplicationController* controller;
	CommandConsole* commandConsole;
	QToolButton* consoleToggleButton;
	ActionPanelWidget* actionPanel = nullptr;
};

GameMainFrame::GameMainFrame(Resources* resources)
: d(new Private(this)) {
	d->resources = resources;
	d->setupConsole();
	d->setupView();

	connect(d->consoleToggleButton, &QToolButton::toggled, this, &GameMainFrame::toggleCommandConsole);

	// Горячая клавиша
	auto toggleAction = new QAction(this);
	toggleAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_QuoteLeft));
	connect(toggleAction, &QAction::triggered, this, [this]() {
		bool newState = !d->commandConsole->isVisible();
		d->consoleToggleButton->setChecked(newState);
		toggleCommandConsole(newState);
		});

	addAction(toggleAction);
}

void GameMainFrame::toggleCommandConsole(bool visible) {
	if (visible) {
		d->commandConsole->showConsole();
	}
	else {
		d->commandConsole->hideConsole();
	}
}
