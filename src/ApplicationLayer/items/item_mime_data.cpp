#include "ApplicationLayer/items/item_mime_data.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include <QIODevice>
#include <QPainter>

ItemMimeData::ItemMimeData() {
}

ItemMimeData::ItemMimeData(const InventoryItemHandler& item) {
	id = item.id;
	name = item.entity->name;
	x = item.x;
	y = item.y;
	width = item.entity->width;
	height = item.entity->height;
	count = item.count;
	maxStack = item.entity->maxStack;
	type = static_cast<qint32>(item.entity->type);
	equipmentType = static_cast<qint32>(item.entity->equipmentType);
}

ItemMimeData ItemMimeData::fromMimeData(const QByteArray& data) {
	QDataStream stream(data);
	ItemMimeData item;

	stream >> item.id >> item.name >> item.x >> item.y >> item.width >> item.height >>
		item.count >> item.maxStack >> item.type >> item.equipmentType;

	return item;
}

