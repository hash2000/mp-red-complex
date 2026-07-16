#pragma once
#include <memory>

class ApplicationController;
class WindowsController;
class ActionPanelController;

class Controllers {
public:
	Controllers(ApplicationController* appController);
	~Controllers();

	WindowsController* windowsController() const;
	ActionPanelController* actionPanelController() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
