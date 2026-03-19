#include "Game/main_frame.h"
#include "Game/commands/command_console.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/controllers.h"
#include "Game/controllers/windows_controller.h"
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
		auto splitter = new QSplitter(Qt::Vertical);
		splitter->addWidget(mdiArea);
		splitter->addWidget(commandConsole);
		splitter->setStretchFactor(0, 1);
		splitter->setStretchFactor(1, 0);
		q->setCentralWidget(splitter);

		controller->executeCommandByName("window-create", QStringList{ "map" });
		controller->executeCommandByName("window-create", QStringList{ "equipment", "abfa5aac-7fb5-4570-965f-00af8aee664a" });
		controller->executeCommandByName("window-create", QStringList{ "inventory", "3f2df95f-581b-4084-94bb-20322325e728" });
		controller->executeCommandByName("window-create", QStringList{ "inventory", "b85cf432-ec9d-441e-b438-eab9c5630e4b" });
		controller->executeCommandByName("window-create", QStringList{ "items", "1d6abf2e-9d77-4cf7-9444-2b54aca14259" });
	}


	GameMainFrame* q;
	Resources* resources;
	MdiArea* mdiArea;
	ApplicationController* controller;
	CommandConsole* commandConsole;
	QToolButton* consoleToggleButton;
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
