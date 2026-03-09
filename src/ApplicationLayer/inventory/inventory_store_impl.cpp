#include "ApplicationLayer/inventory/inventory_store_impl.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "DataLayer/inventory/inventory_data_provider.h"
#include "DataLayer/inventory/inventory_item.h"
#include <QDebug>
#include <map>

class InventoryStoreImpl::Private {
public:
	Private(InventoryStoreImpl* parent)
		: q(parent) {
	}

	InventoryStoreImpl* q;
	InventoryDataProvider* dataProvider;
	ItemsService* itemsService;
	std::map<QUuid, std::unique_ptr<ItemPlacementService>> services;
};

InventoryStoreImpl::InventoryStoreImpl(InventoryDataProvider* dataProvider, ItemsService* itemsService, QObject* parent)
	: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
	d->itemsService = itemsService;
}

InventoryStoreImpl::~InventoryStoreImpl() = default;

ItemPlacementService* InventoryStoreImpl::load(const QUuid& id, bool loadIfNotExists) {
	const auto& it = d->services.find(id);
	if (it == d->services.end() && !loadIfNotExists) {
		return nullptr;
	}

	Inventory inventoryData;
	if (!d->dataProvider->loadInventory(id, inventoryData)) {
		return nullptr;
	}

	auto inventory = std::make_unique<InventoryService>(d->itemsService);
	if (!inventory->load(inventoryData)) {
		return nullptr;
	}

	qDebug() << "Load inventory" << id;

	const auto &insertREsult = d->services.emplace(id, std::move(inventory));

	return insertREsult.first->second.get();
}
