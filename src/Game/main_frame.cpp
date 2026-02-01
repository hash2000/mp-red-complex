#include "Game/main_frame.h"
#include "Game/map_view/map_window.h"
#include "Game/commands/command_console.h"
#include "Game/app_controller.h"
#include "Game/controllers/windows_controller.h"
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
	Private(GameMainFrame* parent, std::shared_ptr<Resources> resources)
	: q(parent)
	, resources(resources) {
		mdiArea = new QMdiArea(parent);
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

	std::shared_ptr<Resources> resources;
	GameMainFrame* q;
	QMdiArea* mdiArea;
	ApplicationController* controller;
	CommandConsole* commandConsole;
	QToolButton* consoleToggleButton;
};

GameMainFrame::GameMainFrame(std::shared_ptr<Resources> resources)
: d(new Private(this, resources)) {
	setupConsole();
	setupMapView();
	setupView();
}

void GameMainFrame::setupMapView() {
	auto widget = new MapWindow;
	auto subWndow = d->mdiArea->addSubWindow(widget);
	d->controller->windowsController()->registerWindow(widget);
	subWndow->setWindowTitle("Word map");
	subWndow->setAttribute(Qt::WA_DeleteOnClose, true);
	subWndow->resize(640, 480);
	subWndow->show();
}

void GameMainFrame::setupConsole() {
	d->controller = new ApplicationController();
	d->commandConsole = new CommandConsole(d->controller, this);
	d->commandConsole->setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint);

	d->controller->windowsController()->setMdiArea(d->mdiArea);

	// Кнопка переключения в статусбаре
	d->consoleToggleButton = new QToolButton(statusBar());
	d->consoleToggleButton->setIcon(QIcon::fromTheme("utilities-terminal", QIcon(":/icons/terminal.png")));
	d->consoleToggleButton->setToolTip("Command Console (Ctrl+`)");
	d->consoleToggleButton->setCheckable(true);
	d->consoleToggleButton->setAutoRaise(true);
	d->consoleToggleButton->setChecked(!d->commandConsole->isVisible());

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

	statusBar()->addPermanentWidget(d->consoleToggleButton);
}

void GameMainFrame::toggleCommandConsole(bool visible) {
	if (visible) {
		d->commandConsole->showConsole();
	}
	else {
		d->commandConsole->hideConsole();
	}
}

void GameMainFrame::setupView() {
	auto splitter = new QSplitter(Qt::Vertical);
	splitter->addWidget(d->mdiArea);
	splitter->addWidget(d->commandConsole);
	splitter->setStretchFactor(0, 1);
	splitter->setStretchFactor(1, 0);
	setCentralWidget(splitter);
}
