#include "ApplicationLayer/controllers/inventory/inventories_controller.h"
#include "ApplicationLayer/services/inventory/inventories_service.h"

class InventoriesController::Private {
public:
	Private(InventoriesController* parent)
		: q(parent) {
	}

	InventoriesController* q;
	InventoriesService* service;
};


InventoriesController::InventoriesController(InventoriesService* service)
	: d(std::make_unique<Private>(this)) {
	d->service = service;
}

InventoriesController::~InventoriesController() = default;

