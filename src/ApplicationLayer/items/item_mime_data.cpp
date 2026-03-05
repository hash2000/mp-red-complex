#include "ApplicationLayer/items/item_mime_data.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include "DataLayer/items/item.h"
#include <QIODevice>
#include <QPainter>

ItemMimeData::ItemMimeData() {
}

ItemMimeData::ItemMimeData(const InventoryItemHandler& item) {
	id = item.id;
	count = item.count;
	owner = static_cast<qint32>(ItemOwner::Inventory);
	coord.pos.x = item.x;
	coord.pos.y = item.y;
	setEntity(*item.entity);
}

ItemMimeData::ItemMimeData(const EquipmentItemHandler& item) {
	id = item.id;
	count = 1;
	owner = static_cast<qint32>(ItemOwner::Equipment);
	coord.slot = static_cast<qint32>(item.slot);
	setEntity(*item.entity);
}

void ItemMimeData::setEntity(const ItemEntity& entity) {
	name = entity.name;
	width = entity.width;
	height = entity.height;
	maxStack = entity.maxStack;
	type = static_cast<qint32>(entity.type);
	equipmentType = static_cast<qint32>(entity.equipmentType);
}

QByteArray ItemMimeData::toMimeData() const {
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);

	stream << id << name << width << height << count << maxStack
		<< type
		<< equipmentType
		<< owner;

	const auto eOwner = static_cast<ItemOwner>(owner);
	switch (eOwner) {
	case ItemOwner::Inventory:
		stream << coord.pos.x << coord.pos.y;
		break;
	case ItemOwner::Equipment:
		stream << coord.slot;
		break;
	}

	return data;
}

ItemMimeData ItemMimeData::fromMimeData(const QByteArray& data) {
	QDataStream stream(data);
	ItemMimeData item;

	stream >> item.id >> item.name >> item.width >> item.height >> item.count >> item.maxStack
		>> item.type
		>> item.equipmentType
		>> item.owner;

	const auto eOwner = static_cast<ItemOwner>(item.owner);
	switch (eOwner) {
	case ItemOwner::Inventory:
		stream >> item.coord.pos.x >> item.coord.pos.y;
		break;
	case ItemOwner::Equipment:
		stream >> item.coord.slot;
		break;
	}

	return item;
}

