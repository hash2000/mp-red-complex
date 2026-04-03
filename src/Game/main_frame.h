#pragma once
#include "Engine/main_frame.h"

class Resources;

class GameMainFrame : public MainFrame {
public:
	GameMainFrame(Resources* resources);
	virtual ~GameMainFrame() = default;

private slots:
	void onToggleCommandConsole(bool visible);
	void onUserLogout();

private:
	class Private;
	std::unique_ptr<Private> d;
};
