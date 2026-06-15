#pragma once
#include "Content/InventroiesModule/models/inventory_item.h"

class ItemEntity;
class Item;

class InventoryItemHandler : public InventoryItem {
public:
	const ItemEntity* entity;
	const Item* item;

public:
	bool compare(const InventoryItemHandler& item) const;
	bool canMergeWith(const InventoryItemHandler& other) const;
};
