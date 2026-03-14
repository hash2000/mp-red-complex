#include "mock_items_data_provider.h"
#include "DataLayer/inventory/inventory_item.h"

// ============================================================================
// MockItemsDataProvider
// ============================================================================

void MockItemsDataProvider::addEntity(const QString& id, const ItemEntity& entity) {
	entities[id] = entity;
}

void MockItemsDataProvider::addItem(const QUuid& id, const Item& item) {
	items[id] = item;
}

bool MockItemsDataProvider::loadEntitiesIds(std::list<QString>& list) const {
	for (const auto& [id, entity] : entities) {
		list.push_back(id);
	}
	return !entities.empty();
}

bool MockItemsDataProvider::loadEntity(const QString& id, ItemEntity& entity) const {
	const auto it = entities.find(id);
	if (it == entities.end()) {
		return false;
	}
	entity = it->second;
	return true;
}

bool MockItemsDataProvider::loadItem(const QUuid& id, Item& item) const {
	const auto it = items.find(id);
	if (it == items.end()) {
		return false;
	}
	item = it->second;
	return true;
}

// ============================================================================
// MockItemPlacementStore
// ============================================================================

MockItemPlacementStore::MockItemPlacementStore(ItemsService* itemsService)
	: itemsService(itemsService) {
}

ItemPlacementService* MockItemPlacementStore::load(const QUuid& id, bool loadIfNotExists) {
	const auto& it = services.find(id);
	if (it != services.end()) {
		return it->second.get();
	}

	// Создаём новый инвентарь с размерами по умолчанию
	return createInventory(id, 10, 10);
}

InventoryService* MockItemPlacementStore::createInventory(const QUuid& id, int rows, int cols) {
	auto inventory = std::make_unique<InventoryService>(itemsService);

	Inventory inv;
	inv.id = id;
	inv.name = id.toString();
	inv.rows = rows;
	inv.cols = cols;

	inventory->load(inv);

	const auto& [it, inserted] = services.emplace(id, std::move(inventory));
	return it->second.get();
}

// ============================================================================
// MockInventoriesDataProvider
// ============================================================================

void MockInventoriesDataProvider::addInventory(const QUuid& id) {
	inventories.push_back(id);
}

void MockInventoriesDataProvider::addEquipment(const QUuid& id) {
	equipments.push_back(id);
}

void MockInventoriesDataProvider::loadInventories(std::list<QUuid>& list) const {
	list = inventories;
}

void MockInventoriesDataProvider::loadEquipments(std::list<QUuid>& list) const {
	list = equipments;
}

// ============================================================================
// MockInventoryDataProvider
// ============================================================================

void MockInventoryDataProvider::addInventory(const QUuid& id, const Inventory& inventory) {
	inventories.emplace(id, inventory);
}

bool MockInventoryDataProvider::loadInventory(const QUuid& id, Inventory& inventory) const {
	const auto it = inventories.find(id);
	if (it == inventories.end()) {
		return false;
	}
	inventory = it->second;
	return true;
}
