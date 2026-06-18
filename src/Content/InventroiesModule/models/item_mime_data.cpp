#include "Content/InventroiesModule/models/item_mime_data.h"
#include "Content/InventroiesModule/models/container.h"
#include "Content/InventroiesModule/models/item.h"
#include <QIODevice>
#include <QPainter>

ItemMimeData::ItemMimeData() {
}

ItemMimeData::ItemMimeData(const ContainerItem& item) {
	id = item.item->id;
	setEntity(*item.item->entity);

	count = item.count;
	x = item.position_base;
	y = item.position_secondary;
}

ItemMimeData::ItemMimeData(const Item& item) {
	id = item.id;
	setEntity(*item.entity);
	count = 1;
	x = 0;
	y = 0;
}

void ItemMimeData::setEntity(const ItemEntity& entity) {
	name = entity.name;
	width = entity.width;
	height = entity.height;
	maxStack = entity.maxStack;
	type = static_cast<qint32>(entity.type);
	subType = static_cast<qint32>(entity.subType);
	entityId = entity.id;
}

QByteArray ItemMimeData::toMimeData() const {
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);

	stream << id << entityId << name << x << y << width << height << count << maxStack
		<< type
		<< subType;

	return data;
}

ItemMimeData ItemMimeData::fromMimeData(const QByteArray& data) {
	QDataStream stream(data);
	ItemMimeData item;

	stream >> item.id >> item.entityId >> item.name
		>> item.x >> item.y >> item.width >> item.height
		>> item.count >> item.maxStack
		>> item.type
		>> item.subType;

	return item;
}

