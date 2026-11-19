#pragma once
#include "Libs/Engine/main_frame.h"

class Resources;
class UserView;

class LauncherMainFrame : public MainFrame {
public:
	LauncherMainFrame(Resources* resources);
	virtual ~LauncherMainFrame();

private slots:
	void onToggleCommandConsole(bool visible);
	void onUserLogin(const UserView& user);
	void onUserLogout();

private:
	class Private;
	std::unique_ptr<Private> d;
};
