#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/items_placement_store.h"
#include "ApplicationLayer/items_placement_service.h"
#include "ApplicationLayer/inventory/inventory_store_impl.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/equipment/equipment_store_impl.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include "DataLayer/inventories/inventories_data_provider.h"
#include "DataLayer/inventory/inventory_data_provider.h"
#include "DataLayer/items/items_data_provider.h"
#include <QDebug>
#include <map>

class InventoriesService::Private {
public:
	Private(InventoriesService* parent)
		: q(parent) {
	}

	void loadInventories() {
		std::list<QUuid> items;
		inventoriesDataProvider->loadInventories(items);

		for (const auto& item : items) {
			stores.emplace(item, std::make_unique<InventoryStoreImpl>(inventoryDataProvider, itemsService));
		}
	}

	void loadEquipments() {
		std::list<QUuid> items;
		inventoriesDataProvider->loadEquipments(items);

		for (const auto& item : items) {
			stores.emplace(item, std::make_unique<EquipmentStoreImpl>(equipmentDataProvider, itemsService));
		}
	}

	InventoriesService* q;
	std::map<QUuid, std::unique_ptr<ItemPlacementStore>> stores;
	InventoriesDataProvider* inventoriesDataProvider;
	InventoryDataProvider* inventoryDataProvider;
	EquipmentDataProvider* equipmentDataProvider;
	ItemsService* itemsService;
};

InventoriesService::InventoriesService(
	InventoriesDataProvider* inventoriesDataProvider,
	InventoryDataProvider* inventoryDataProvider,
	EquipmentDataProvider* equipmentDataProvider,
	ItemsService* itemsService,
	QObject* parent)
: d(std::make_unique<Private>(this)) {
	d->inventoriesDataProvider = inventoriesDataProvider;
	d->inventoryDataProvider = inventoryDataProvider;
	d->equipmentDataProvider = equipmentDataProvider;
	d->itemsService = itemsService;
}

InventoriesService::~InventoriesService() = default;

void InventoriesService::load() {
	d->loadInventories();
	d->loadEquipments();
}

ItemPlacementService* InventoriesService::placementService(const QUuid& id, bool loadIfNotExists) const {
	const auto& it = d->stores.find(id);
	if (it == d->stores.end()) {
		qWarning() << "Inventory loader not found" << id.toString();
		return nullptr;
	}

	auto service = it->second->load(id, loadIfNotExists);
	return service;
}

bool InventoriesService::moveItem(const ItemMimeData& item, int col, int row, const QUuid& fromId, const QUuid& toId) {
	auto fromService = placementService(fromId, false);
	auto toService = placementService(toId, false);

	if (!fromService || !toService) {
		qWarning() << "moveItem: service not found"
			<< "from:" << fromId << "to:" << toId;
		return false;
	}

	ItemMimeData changedItem = item;
	if (!fromService->removeItemsFromStack(changedItem)) {
		return false;
	}

	if (changedItem.count == 0) {
		emit itemMoved(item, fromId, toId);
		return true;
	}

	changedItem.x = col;
	changedItem.y = row;

	if (!toService->applyDublicateFromItem(changedItem)) {
		return false;
	}

	emit itemMoved(item, fromId, toId);
	return true;
}

std::list<QUuid> InventoriesService::getAllIds() const {
	std::list<QUuid> ids;
	for (const auto& [id, store] : d->stores) {
		ids.push_back(id);
	}
	return ids;
}

InventoryService* InventoriesService::getInventoryService(const QUuid& id) const {
	const auto& it = d->stores.find(id);
	if (it == d->stores.end()) {
		return nullptr;
	}

	// Проверяем, что это InventoryStoreImpl
	auto inventoryStore = dynamic_cast<InventoryStoreImpl*>(it->second.get());
	if (!inventoryStore) {
		return nullptr;
	}

	// Загружаем сервис (если ещё не загружен)
	auto service = inventoryStore->load(id, false);
	return dynamic_cast<InventoryService*>(service);
}

EquipmentService* InventoriesService::getEquipmentService(const QUuid& id) const {
	const auto& it = d->stores.find(id);
	if (it == d->stores.end()) {
		return nullptr;
	}

	// Проверяем, что это EquipmentStoreImpl
	auto equipmentStore = dynamic_cast<EquipmentStoreImpl*>(it->second.get());
	if (!equipmentStore) {
		return nullptr;
	}

	// Загружаем сервис (если ещё не загружен)
	auto service = equipmentStore->load(id, false);
	return dynamic_cast<EquipmentService*>(service);
}
