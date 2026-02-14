#include "DataLayer/inventory/inventory_item.h"
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

	// Иконка-заглушка
	item.icon = QPixmap(64, 64);
	item.icon.fill(Qt::darkCyan);
	QPainter painter(&item.icon);
	painter.setPen(Qt::white);
	painter.drawText(item.icon.rect(), Qt::AlignCenter, id.left(3));

	return item;
}

std::shared_ptr<InventoryItem> InventoryItem::fromJson(const QJsonObject& json) {
	auto item = std::make_shared<InventoryItem>();
	item->id = QUuid::createUuid()
		.toString(QUuid::StringFormat::WithoutBraces);
	item->entityId = json["id"].toString();
	item->name = json["name"].toString();
	item->description = json["description"].toString();
	item->iconPath = json["icon"].toString();

	// Тип предмета
	QString typeStr = json["type"].toString().toLower();
	if (typeStr == "equipment") {
		item->type = InventoryItemType::Equipment;
		QString equipType = json["equipmentType"].toString().toLower();
		if (equipType == "head") item->equipmentType = EquipmentItemType::Head;
		else if (equipType == "body") item->equipmentType = EquipmentItemType::Body;
		else if (equipType == "weapon") item->equipmentType = EquipmentItemType::Weapon;
		else if (equipType == "shield") item->equipmentType = EquipmentItemType::Shield;
		else if (equipType == "gloves") item->equipmentType = EquipmentItemType::Gloves;
		else if (equipType == "boots") item->equipmentType = EquipmentItemType::Boots;
		else if (equipType == "ring") item->equipmentType = EquipmentItemType::Ring;
		else if (equipType == "amulet") item->equipmentType = EquipmentItemType::Amulet;
	}
	else if (typeStr == "resource") {
		item->type = InventoryItemType::Resource;
		item->maxStack = json["maxStack"].toInt(100);
		item->count = json["count"].toInt(0);
	}
	else if (typeStr == "component") {
		item->type = InventoryItemType::Component;
		item->maxStack = json["maxStack"].toInt(100);
		item->count = json["count"].toInt(0);
	}
	else if (typeStr == "container") {
		item->type = InventoryItemType::Container;
		QJsonObject cap = json["capacity"].toObject();
		item->containerCapacity = InventoryItemContainerCapacity{
				cap["rows"].toInt(4),
				cap["cols"].toInt(4)
		};
	}

	// Размер в ячейках
	item->width = json["width"].toInt(1);
	item->height = json["height"].toInt(1);

	// Рецепт крафта (опционально)
	if (json.contains("recipe")) {
		item->recipe = InventoryItemRecipe();
		QJsonArray ingredients = json["recipe"]
			.toObject()["ingredients"]
			.toArray();

		for (const QJsonValue& ingVal : ingredients) {
			QJsonObject ingObj = ingVal.toObject();
			item->recipe->ingredients.push_back({
					ingObj["itemId"].toString(),
					ingObj["amount"].toInt()
				});
		}
	}

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
	item->containerCapacity = containerCapacity;
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
