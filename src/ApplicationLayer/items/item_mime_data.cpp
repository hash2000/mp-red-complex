#include "ApplicationLayer/items/item_mime_data.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include <QIODevice>
#include <QPainter>

ItemMimeData::ItemMimeData() {
}

ItemMimeData::ItemMimeData(const InventoryItemHandler& item) {
	id = item.id;
	count = item.count;
	owner = static_cast<qint32>(ItemOwner::Inventory);
	x = item.x;
	y = item.y;
	setEntity(*item.entity);
}

ItemMimeData::ItemMimeData(const EquipmentItemHandler& item) {
	id = item.id;
	count = 1;
	owner = static_cast<qint32>(ItemOwner::Equipment);

	const auto position = item.slotToPosition();
	x = position.x();
	y = position.y();
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

	stream << id << name << x << y << width << height << count << maxStack
		<< type
		<< equipmentType
		<< owner;

	return data;
}

ItemMimeData ItemMimeData::fromMimeData(const QByteArray& data) {
	QDataStream stream(data);
	ItemMimeData item;

	stream >> item.id >> item.name >> item.x >> item.y >> item.width >> item.height >> item.count >> item.maxStack
		>> item.type
		>> item.equipmentType
		>> item.owner;

	return item;
}

