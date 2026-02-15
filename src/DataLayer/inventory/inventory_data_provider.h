#pragma once
#include "DataFormat/proto/inventory_item.h"
#include <QList>
#include <QUuid>
#include <memory>

struct InventoryDataProvider {
	virtual ~InventoryDataProvider() = default;

	virtual std::shared_ptr<Inventory> loadInventory(const QUuid& id) const = 0;
};
