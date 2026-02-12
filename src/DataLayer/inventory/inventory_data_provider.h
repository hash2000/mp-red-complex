#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QList>
#include <QUuid>
#include <memory>

struct InventoryDataProvider {
	virtual ~InventoryDataProvider() = default;

	virtual std::shared_ptr<Inventory> loadInventory(const QUuid& id) const = 0;
	virtual std::shared_ptr<InventoryItem> loadItem(const QString& id) const = 0;
	virtual QPixmap loadIcon(const InventoryItem& item) const = 0;
};
