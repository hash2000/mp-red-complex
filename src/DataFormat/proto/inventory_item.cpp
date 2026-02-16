#include "DataFormat/proto/inventory_item.h"
#include <QIODevice>
#include <QPainter>

QByteArray InventoryHandler::toMimeData() const {
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);

	stream << id << name << x << y << width << height << count << maxStack << static_cast<qint32>(type);

	return data;
}

InventoryHandler InventoryHandler::fromMimeData(const QByteArray& data) {
	QDataStream stream(data);
	QString id, name;
	qint32 x, y, width, height, count, maxStack, type;
	stream >> id >> name >> x >> y >> width >> height >> count >> maxStack >> type;

	// В реальном проекте здесь нужно запросить полные данные из ItemDatabase
	// Для примера возвращаем заглушку:
	InventoryHandler item;
	item.id = id;
	item.name = name;
	item.x = x;
	item.y = y;
	item.width = width;
	item.height = height;
	item.count = count;
	item.maxStack = maxStack;
	item.type = static_cast<InventoryItemType>(type);
	return item;
}

std::shared_ptr<InventoryItem> InventoryItem::duplicate(bool newId) const {
	auto item = std::make_shared<InventoryItem>();

	if (newId) {
		// дубликат, но с новым идентификатором
		item->id = QUuid::createUuid()
			.toString(QUuid::StringFormat::WithoutBraces);
	}
	else {
		item->id = id;
	}

	item->entityId = entityId;
	item->name = name;
	item->description = description;
	item->iconPath = iconPath;
	item->type = type;
	item->equipmentType = equipmentType;
	item->maxStack = maxStack;
	item->count = count;
	item->width = width;
	item->height = height;
	item->recipe = recipe;
	item->rarity = rarity;
	item->icon = icon.copy();
	return item;
}

bool InventoryItem::compare(const InventoryHandler& item) {
	// width и height тут стоит проверять, потому что элемент инвентаря можно поворачивать
	// тут это именно для проверки, повёрнута копия или нет
	return item.x == x &&
		item.y == y &&
		item.width == width &&
		item.height == height &&
		item.id == id;
}

bool InventoryItem::canMergeWith(const InventoryItem& other) const {
	return id == other.id &&
		count + other.count <= maxStack &&
		type == other.type;
}
