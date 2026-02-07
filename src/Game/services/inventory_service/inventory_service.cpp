#include "Game/services/inventory_service/inventory_service.h"

class InventoryService::Private {
public:
	Private(InventoryService* parent)
	: q(parent) {
	}

	InventoryService* q;
	InventoryDataProvider* dataProvider;
};

InventoryService::InventoryService(InventoryDataProvider* dataProvider, QObject* parent)
: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
}

InventoryService::~InventoryService() = default;

QList<InventoryItem> InventoryService::fromContainer(const QUuid& id) const {
	return d->dataProvider->fromContainer(id);
}

void InventoryService::onSave() {

}

void InventoryService::onLoad() {

}
