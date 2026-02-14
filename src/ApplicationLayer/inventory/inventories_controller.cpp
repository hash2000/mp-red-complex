#include "ApplicationLayer/inventory/inventories_controller.h"
#include <QUuid>
#include <set>

class InventoriesController::Private {
public:
	Private(InventoriesController* parent)
		: q(parent) {
	}

	InventoriesController* q;
	std::set<QUuid> inventoriesLoaded;
};


InventoriesController::InventoriesController()
	: d(std::make_unique<Private>(this)) {
}

InventoriesController::~InventoriesController() = default;

bool InventoriesController::postInventoryLoad(const QUuid& id) {
	const auto & result = d->inventoriesLoaded.emplace(id);
	return result.second;
}

void InventoriesController::postInventoryLoaded(const QUuid& id) {
	emit inventoryLoaded(id);
}

void InventoriesController::postInventoryUnloaded(const QUuid& id) {
	const auto & result = d->inventoriesLoaded.erase(id);
	if (result != 0) {
		emit inventoryClosed(id);
	}
}
