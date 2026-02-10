#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QList>
#include <QUuid>
#include <optional>

struct InventoryDataProvider {
	virtual ~InventoryDataProvider() = default;

	virtual std::optional<Inventory> fromContainer(const QUuid& id) const = 0;
	virtual std::optional<InventoryItem> getItem(const QString& id) const = 0;
	virtual QPixmap loadIcon(const InventoryItem& item) const = 0;
};
