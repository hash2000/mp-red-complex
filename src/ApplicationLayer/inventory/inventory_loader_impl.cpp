#include "ApplicationLayer/inventory/inventory_loader_impl.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "DataLayer/inventory/inventory_data_provider.h"
#include "DataLayer/inventory/inventory_item.h"
#include <QDebug>
#include <map>

class InventoryLoaderImpl::Private {
public:
	Private(InventoryLoaderImpl* parent)
		: q(parent) {
	}

	InventoryLoaderImpl* q;
	InventoryDataProvider* dataProvider;
	ItemsService* itemsService;
};

InventoryLoaderImpl::InventoryLoaderImpl(InventoryDataProvider* dataProvider, ItemsService* itemsService, QObject* parent)
	: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
	d->itemsService = itemsService;
}

InventoryLoaderImpl::~InventoryLoaderImpl() = default;

std::unique_ptr<ItemPlacement> InventoryLoaderImpl::load(const QUuid& id) {
	Inventory inventoryData;
	if (!d->dataProvider->loadInventory(id, inventoryData)) {
		return nullptr;
	}

	auto inventory = std::make_unique<InventoryService>(d->itemsService);
	if (!inventory->load(inventoryData)) {
		return nullptr;
	}

	qDebug() << "Load inventory" << id;

	return inventory;
}
