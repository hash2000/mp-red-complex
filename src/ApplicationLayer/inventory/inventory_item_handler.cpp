#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include <QIODevice>

bool InventoryItemHandler::compare(const InventoryItemHandler& item) const {
	// width и height тут стоит проверять, потому что элемент инвентаря можно поворачивать
	// тут это именно для проверки, повёрнута копия или нет
	return item.x == x &&
		item.y == y &&
		item.entity->width == entity->width &&
		item.entity->height == entity->height &&
		item.id == id;
}

bool InventoryItemHandler::canMergeWith(const InventoryItemHandler& other) const {
	return id == other.id &&
		count + other.count <= entity->maxStack &&
		entity->type == other.entity->type;
}
