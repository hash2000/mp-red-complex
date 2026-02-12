#include "Game/services/inventory/inventories_service.h"
#include "Game/services/inventory/inventory_service.h"
#include <map>

class InventoriesService::Private {
public:
	Private(InventoriesService* parent)
	: q(parent) {
	}

	InventoriesService* q;
	InventoryDataProvider* dataProvider;
	std::map<QUuid, std::unique_ptr<InventoryService>> inventories;
};

InventoriesService::InventoriesService(InventoryDataProvider* dataProvider, QObject* parent)
: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
}

InventoriesService::~InventoriesService() = default;

InventoryService* InventoriesService::inventoryService(const QUuid& id) const {
	if (!d->inventories.contains(id)) {
		auto inventory = d->dataProvider->loadInventory(id);
		d->inventories[id] = std::make_unique<InventoryService>(inventory);
	}

	return d->inventories[id].get();
}

void InventoriesService::onSave() {

}

void InventoriesService::onLoad() {

}
