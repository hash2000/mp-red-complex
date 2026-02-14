#pragma once
#include <memory>

class Services;
class WindowsController;
class InventoriesController;

class Controllers {
public:
	Controllers(Services* services);
	~Controllers();

	WindowsController* windowsController() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
