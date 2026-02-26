#pragma once
#include "DataLayer/inventory/inventory_item.h"

class InventoryItemHandler : public InventoryItem {
public:
	const ItemEntity* entity;

public:
	bool compare(const InventoryItemHandler& item) const;
	bool canMergeWith(const InventoryItemHandler& other) const;
	QByteArray toMimeData() const;
};
