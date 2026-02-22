#include "ApplicationLayer/inventory/inventory_item_mime_data.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include <QIODevice>
#include <QPainter>

InventoryItemMimeData::InventoryItemMimeData() {
}

InventoryItemMimeData::InventoryItemMimeData(const InventoryItemHandler& item) {
	id = item.id;
	name = item.entity->name;
	x = item.x;
	y = item.y;
	width = item.entity->width;
	height = item.entity->height;
	count = item.count;
	maxStack = item.entity->maxStack;
	type = static_cast<qint32>(item.entity->type);
}

InventoryItemMimeData InventoryItemMimeData::fromMimeData(const QByteArray& data) {
	QDataStream stream(data);
	QString id, name;
	qint32 x, y, width, height, count, maxStack, type;
	stream >> id >> name >> x >> y >> width >> height >> count >> maxStack >> type;

	// В реальном проекте здесь нужно запросить полные данные из ItemDatabase
	// Для примера возвращаем заглушку:
	InventoryItemMimeData item;
	item.id = id;
	item.name = name;
	item.x = x;
	item.y = y;
	item.width = width;
	item.height = height;
	item.count = count;
	item.maxStack = maxStack;
	item.type = type;
	return item;
}

