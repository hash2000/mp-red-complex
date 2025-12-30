#pragma once
#include "Launcher/widgets/side_bar/side_bar.h"
#include "Launcher/widgets/game_view/game_view.h"
#include "Launcher/widgets/profile/profile_view.h"
#include "Engine/main_frame.h"
#include "Resources/resources.h"
#include <QStackedWidget>
#include <memory>


class LauncherMainFrame : public MainFrame {
public:
	LauncherMainFrame(std::shared_ptr<Resources> resources);

	virtual ~LauncherMainFrame() = default;

private:
	void setupView();
	void setupSideBarViews();
	void setupSideBar();

private slots:
	void onSideBarAction(SideBarAction* action);

private:
	std::shared_ptr<Resources> _resources;
	QStackedWidget* _center;
	SideBar* _sideBar;
	GameView* _gameView;
	ProfileView* _profileView;
};
