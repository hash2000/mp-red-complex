#pragma once
#include <QUuid>

class Inventory;

class InventoryDataProvider {
public:
	virtual ~InventoryDataProvider() = default;

	virtual bool loadInventory(const QUuid& id, Inventory& inventory) const = 0;
};
