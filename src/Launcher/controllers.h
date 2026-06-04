#pragma once
#include <memory>

class Services;
class ApplicationController;
class WindowsController;
class InventoriesController;
class ActionPanelController;

class Controllers {
public:
	Controllers(ApplicationController* appController, Services* services);
	~Controllers();

	WindowsController* windowsController() const;
	ActionPanelController* actionPanelController() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
