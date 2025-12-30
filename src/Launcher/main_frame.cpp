#include "Launcher/main_frame.h"
#include <QLabel>

LauncherMainFrame::LauncherMainFrame(std::shared_ptr<Resources> resources)
	: _resources(resources)
	, _center(new QStackedWidget(this))
	, _gameView(new GameView(qRgb(20, 20, 50), this))
	, _profileView(new ProfileView(this))
	, _sideBar(new SideBar(this)) {
	setupView();
	setupSideBarViews();
	setupSideBar();
}

void LauncherMainFrame::setupView() {
	auto* central = new QWidget(this);
	setCentralWidget(central);

	auto* mainLayout = new QHBoxLayout(central);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	mainLayout->addWidget(_center, 1);
	mainLayout->addWidget(_sideBar);

	setWindowTitle("RedComplex");
}

void LauncherMainFrame::setupSideBarViews() {
	_center->addWidget(_gameView);
	_center->addWidget(_profileView);
	_center->setCurrentWidget(_gameView);
}

void LauncherMainFrame::setupSideBar() {
	_sideBar->addAction(_profileView);
	_sideBar->addAction(_gameView);
	_sideBar->setButtonChecked(_gameView->id(), true);

	connect(_sideBar, &SideBar::onAction, this, &LauncherMainFrame::onSideBarAction);
}

void LauncherMainFrame::onSideBarAction(SideBarAction* action) {
	switch (action->viewState()) {
	case ViewState::UserProfile:
		_center->setCurrentWidget(_profileView);
		break;
	case ViewState::Game:
		_center->setCurrentWidget(_gameView);
		break;
	}
}
