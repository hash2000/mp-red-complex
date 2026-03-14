#pragma once
#include "ApplicationLayer/items_placement_store.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "DataLayer/items/items_data_provider.h"
#include "DataLayer/inventory/inventory_data_provider.h"
#include "DataLayer/inventories/inventories_data_provider.h"
#include <QObject>
#include <map>
#include <QUuid>

/// Моковый ItemsDataProvider для юнит-тестов
/// Позволяет задавать тестовые данные программно без файлов
class MockItemsDataProvider : public ItemsDataProvider {
public:
	MockItemsDataProvider() = default;
	~MockItemsDataProvider() override = default;

	// Добавить тестовую сущность предмета
	void addEntity(const QString& id, const ItemEntity& entity);

	// Добавить тестовый предмет
	void addItem(const QUuid& id, const Item& item);

	// Реализация интерфейса ItemsDataProvider
	bool loadEntitiesIds(std::list<QString>& list) const override;
	bool loadEntity(const QString& id, ItemEntity& entity) const override;
	bool loadItem(const QUuid& id, Item& item) const override;

private:
	std::map<QString, ItemEntity> entities;
	std::map<QUuid, Item> items;
};

/// Мок для ItemPlacementStore
/// Возвращает InventoryService для любого запрошенного ID
class MockItemPlacementStore : public ItemPlacementStore {
public:
	MockItemPlacementStore(ItemsService* itemsService);
	~MockItemPlacementStore() override = default;

	ItemPlacementService* load(const QUuid& id, bool loadIfNotExists) override;

	// Создать и вернуть InventoryService для указанного ID
	InventoryService* createInventory(const QUuid& id, int rows = 10, int cols = 10);

private:
	ItemsService* itemsService;
	std::map<QUuid, std::unique_ptr<InventoryService>> services;
};

/// Мок для InventoriesDataProvider
/// Возвращает список зарегистрированных инвентарей
class MockInventoriesDataProvider : public InventoriesDataProvider {
public:
	MockInventoriesDataProvider() = default;
	~MockInventoriesDataProvider() override = default;

	// Добавить инвентарь в список доступных
	void addInventory(const QUuid& id);

	// Добавить экипировку в список доступных
	void addEquipment(const QUuid& id);

	void loadInventories(std::list<QUuid>& list) const override;
	void loadEquipments(std::list<QUuid>& list) const override;

private:
	std::list<QUuid> inventories;
	std::list<QUuid> equipments;
};

/// Мок для InventoryDataProvider
/// Возвращает заранее заданные данные инвентаря
class MockInventoryDataProvider : public InventoryDataProvider {
public:
	MockInventoryDataProvider() = default;
	~MockInventoryDataProvider() override = default;

	// Добавить данные инвентаря
	void addInventory(const QUuid& id, const Inventory& inventory);

	bool loadInventory(const QUuid& id, Inventory& inventory) const override;

private:
	std::map<QUuid, Inventory> inventories;
};
