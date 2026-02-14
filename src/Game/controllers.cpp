#include "Game/controllers.h"
#include "Game/services.h"
#include "Game/controllers/windows_controller.h"
#include "ApplicationLayer/inventory/inventories_controller.h"

class Controllers::Private {
public:
	Private(Controllers* parent) :q(parent) {
	}

	Controllers* q;
	std::unique_ptr<WindowsController> windowsController;
};


Controllers::Controllers(Services* services)
: d(std::make_unique<Private>(this)) {
	d->windowsController = std::make_unique<WindowsController>();
}

Controllers::~Controllers() = default;


WindowsController* Controllers::windowsController() const {
	return d->windowsController.get();
}

