#include "Game/controllers.h"
#include "Game/controllers/windows_controller.h"

class Controllers::Private {
public:
	Private(Controllers* parent) :q(parent) {
	}

	Controllers* q;
	std::unique_ptr<WindowsController> windowsController;
};


Controllers::Controllers()
: d(std::make_unique<Private>(this)) {
	d->windowsController = std::make_unique<WindowsController>();
}

Controllers::~Controllers() = default;


WindowsController* Controllers::windowsController() const {
	return d->windowsController.get();
}
