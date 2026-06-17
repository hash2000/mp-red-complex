#pragma once
#include <QUuid>

class Inventory;

class IInventoryDataWriter {
public:
	virtual ~IInventoryDataWriter() = default;

	virtual bool saveInventory(const QUuid& id, const Inventory& inventory) const = 0;
};
