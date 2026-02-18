#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QList>
#include <QUuid>
#include <memory>

struct InventoryDataProvider {
	virtual ~InventoryDataProvider() = default;

	virtual bool loadInventory(const QUuid& id, InventoryRaw& inventory) const = 0;
};
