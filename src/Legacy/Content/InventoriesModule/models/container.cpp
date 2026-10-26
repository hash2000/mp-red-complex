#include "Content/InventoriesModule/models/container.h"
#include "Content/InventoriesModule/models/item.h"
#include <QIODevice>

bool ContainerItem::compare(const ContainerItem& other) const {
	// width и height тут стоит проверять, потому что элемент инвентаря можно поворачивать
	// тут это именно для проверки, повёрнута копия или нет
	return other.position_base == position_base &&
		other.position_secondary == position_secondary &&
		other.item->entity->width == item->entity->width &&
		other.item->entity->height == item->entity->height &&
		other.item->id == item->id;
}

bool ContainerItem::canMergeWith(const ContainerItem& other) const {
	return other.item->id == item->id &&
		count + other.count <= item->entity->maxStack &&
		item->entity->type == other.item->entity->type;
}

QPoint ContainerItem::slotToPosition() const {
	return convertSlotToPosition(convertPositionToSlot(position_base, position_secondary));
}

QPoint ContainerItem::convertSlotToPosition(ItemSlotType slot) {
	return QPoint(static_cast<qint32>(slot), 0);
}

ItemSlotType ContainerItem::convertPositionToSlot(int col, int row) {
	return static_cast<ItemSlotType>(row);
}
