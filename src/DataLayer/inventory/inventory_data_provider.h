#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QList>
#include <QUuid>

struct InventoryDataProvider {
	virtual ~InventoryDataProvider() = default;

	virtual QList<InventoryItem> fromContainer(const QUuid& id) const = 0;
};
