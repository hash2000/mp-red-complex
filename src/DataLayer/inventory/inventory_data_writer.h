#pragma once
#include <QUuid>

class Inventory;

class InventoryDataWriter {
public:
	virtual ~InventoryDataWriter() = default;

	virtual bool saveInventory(const QUuid& id, const Inventory& inventory) const = 0;
};
