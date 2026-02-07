#include "Game/widgets/inventory/inventory_item.h"
#include <QDataStream>
#include <QFile>
#include <QImage>
#include <QPainter>


InventoryItem InventoryItem::fromJson(const QJsonObject& json) {
	InventoryItem item;
	item.id = json["id"].toString();
	item.name = json["name"].toString();
	item.description = json["description"].toString();

	// Загрузка иконки (путь относительно ресурсов)
	QString iconPath = json["icon"].toString();
	if (!iconPath.isEmpty()) {
		item.icon = QPixmap(iconPath);
		if (item.icon.isNull()) {
			// Фолбэк для отладки
			item.icon = QPixmap(64, 64);
			item.icon.fill(Qt::darkGray);
			QPainter painter(&item.icon);
			painter.setPen(Qt::white);
			painter.drawText(item.icon.rect(), Qt::AlignCenter, item.id.left(2));
		}
	}

	// Тип предмета
	QString typeStr = json["type"].toString().toLower();
	if (typeStr == "equipment") {
		item.type = InventoryItemType::Equipment;
		QString equipType = json["equipmentType"].toString().toLower();
		if (equipType == "head") item.equipmentType = Item::Type::Head;
		else if (equipType == "body") item.equipmentType = Item::Type::Body;
		else if (equipType == "weapon") item.equipmentType = Item::Type::Weapon;
		else if (equipType == "shield") item.equipmentType = Item::Type::Shield;
		else if (equipType == "gloves") item.equipmentType = Item::Type::Gloves;
		else if (equipType == "boots") item.equipmentType = Item::Type::Boots;
		else if (equipType == "ring") item.equipmentType = Item::Type::Ring;
		else if (equipType == "amulet") item.equipmentType = Item::Type::Amulet;
	}
	else if (typeStr == "resource") {
		item.type = InventoryItemType::Resource;
		item.maxStack = json["maxStack"].toInt(999);
	}
	else if (typeStr == "component") {
		item.type = InventoryItemType::Component;
		item.maxStack = json["maxStack"].toInt(100);
	}
	else if (typeStr == "container") {
		item.type = InventoryItemType::Container;
		QJsonObject cap = json["capacity"].toObject();
		item.containerCapacity = ContainerCapacity{
				cap["rows"].toInt(4),
				cap["cols"].toInt(4)
		};
	}

	// Размер в ячейках
	item.width = json["width"].toInt(1);
	item.height = json["height"].toInt(1);

	// Рецепт крафта (опционально)
	if (json.contains("recipe")) {
		item.recipe = Recipe();
		QJsonArray ingredients = json["recipe"].toObject()["ingredients"].toArray();
		for (const QJsonValue& ingVal : ingredients) {
			QJsonObject ingObj = ingVal.toObject();
			item.recipe->ingredients.push_back({
					ingObj["itemId"].toString(),
					ingObj["amount"].toInt()
				});
		}
	}

	return item;
}

bool InventoryItem::canPlaceAt(int gridWidth, int x, int y) const {
	return (x >= 0 && y >= 0 &&
		x + width <= gridWidth &&
		y + height <= 1000); // Высота проверяется отдельно в сетке
}

QByteArray InventoryItem::toMimeData() const {
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);

	stream << id << name << static_cast<qint32>(type) << width << height << count;

	if (type == InventoryItemType::Equipment && equipmentType.has_value()) {
		stream << static_cast<qint32>(*equipmentType);
	}

	return data;
}

std::optional<InventoryItem> InventoryItem::fromMimeData(const QByteArray& data) {
	QDataStream stream(data);
	QString id, name;
	qint32 typeInt, width, height, count;
	stream >> id >> name >> typeInt >> width >> height >> count;

	// В реальном проекте здесь нужно запросить полные данные из ItemDatabase
	// Для примера возвращаем заглушку:
	InventoryItem item;
	item.id = id;
	item.name = name;
	item.type = static_cast<InventoryItemType>(typeInt);
	item.width = width;
	item.height = height;
	item.count = count;

	// Иконка-заглушка
	item.icon = QPixmap(64, 64);
	item.icon.fill(Qt::darkCyan);
	QPainter painter(&item.icon);
	painter.setPen(Qt::white);
	painter.drawText(item.icon.rect(), Qt::AlignCenter, id.left(3));

	return item;
}
