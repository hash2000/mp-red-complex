#pragma once
#include <QList>
#include <QUuid>
#include <memory>

class Inventory;

class InventoryDataProvider {
public:
	virtual ~InventoryDataProvider() = default;

	virtual bool loadInventory(const QUuid& id, Inventory& inventory) const = 0;
};
