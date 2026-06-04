#pragma once
#include "Engine/main_frame.h"

class Resources;
class UserData;

class LauncherMainFrame : public MainFrame {
public:
	LauncherMainFrame(Resources* resources);
	virtual ~LauncherMainFrame();

private slots:
	void onToggleCommandConsole(bool visible);
	void onUserLogin(const UserData& user);
	void onUserLogout();

private:
	class Private;
	std::unique_ptr<Private> d;
};
