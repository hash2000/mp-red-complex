#pragma once
#include <QUuid>

class Inventory;

class IInventoryDataProvider {
public:
	virtual ~IInventoryDataProvider() = default;

	virtual bool loadInventory(const QUuid& id, Inventory& inventory) const = 0;
};
