#pragma once
#include "DataLayer/inventory/inventory_item.h"

class InventoryItemHandler : public InventoryItem {
public:

	bool compare(const InventoryItemHandler& item) const;
	bool canMergeWith(const InventoryItemHandler& other) const;

public:
	const ItemEntity* entity;

	QByteArray toMimeData() const;
};
