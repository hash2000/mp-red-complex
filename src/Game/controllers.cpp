#include "Game/controllers.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/controllers/windows_controller.h"
#include "Game/controllers/action_panel_controller.h"

class Controllers::Private {
public:
	Private(Controllers* parent) :q(parent) {
	}

	Controllers* q;
	std::unique_ptr<WindowsController> windowsController;
	std::unique_ptr<ActionPanelController> actionPanelController;
};


Controllers::Controllers(ApplicationController* appController, Services* services)
: d(std::make_unique<Private>(this)) {
	d->windowsController = std::make_unique<WindowsController>();
	d->actionPanelController = std::make_unique<ActionPanelController>(appController);
}

Controllers::~Controllers() = default;


WindowsController* Controllers::windowsController() const {
	return d->windowsController.get();
}

ActionPanelController* Controllers::actionPanelController() const {
	return d->actionPanelController.get();
}

