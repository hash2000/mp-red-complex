#include "DataLayer/items/item.h"
#include <QIODevice>
#include <QUuid>

QByteArray Item::toMimeData() {
	QByteArray data;
	QDataStream stream(&data, QIODevice::WriteOnly);

	stream << id << name << x << y << width << height << count << maxStack << static_cast<qint32>(type);

	return data;
}

Item Item::fromMimeData(const QByteArray& data) {
	QDataStream stream(data);
	QString id, name;
	qint32 x, y, width, height, count, maxStack, type;
	stream >> id >> name >> x >> y >> width >> height >> count >> maxStack >> type;

	// В реальном проекте здесь нужно запросить полные данные из ItemDatabase
	// Для примера возвращаем заглушку:
	Item item;
	item.id = id;
	item.name = name;
	item.x = x;
	item.y = y;
	item.width = width;
	item.height = height;
	item.count = count;
	item.maxStack = maxStack;
	item.type = static_cast<ItemType>(type);
	return item;
}

bool Item::compare(const Item& other) {
	// width и height тут стоит проверять, потому что элемент инвентаря можно поворачивать
	// тут это именно для проверки, повёрнута копия или нет
	return other.x == x &&
		other.y == y &&
		other.width == width &&
		other.height == height &&
		other.id == id;
}

bool Item::canMergeWith(const Item& other) {
	return count + other.count <= maxStack &&
		type == other.type;
}

void Item::duplicate(Item& item, bool newId) {
	if (newId) {
		// дубликат, но с новым идентификатором
		item.id = QUuid::createUuid()
			.toString(QUuid::StringFormat::WithoutBraces);
	}
	else {
		item.id = id;
	}

	item.entityId = entityId;
	item.name = name;
	item.description = description;
	item.iconPath = iconPath;
	item.type = type;
	item.equipmentType = equipmentType;
	item.maxStack = maxStack;
	item.count = count;
	item.width = width;
	item.height = height;
	item.recipe = recipe;
	item.rarity = rarity;
	item.icon = icon.copy();
}

bool Item::canAcceptEquipment(const ItemEquipmentSlotType& slotType) {
	switch (slotType) {
	case ItemEquipmentSlotType::Head:
		return equipmentType == ItemEquipmentType::Head;
	case ItemEquipmentSlotType::Body:
		return equipmentType == ItemEquipmentType::Body;
	case ItemEquipmentSlotType::WeaponLeft:
		return equipmentType == ItemEquipmentType::Weapon || equipmentType == ItemEquipmentType::Shield;
	case ItemEquipmentSlotType::WeaponRight:
		return equipmentType == ItemEquipmentType::Weapon;
	case ItemEquipmentSlotType::GlovesLeft:
	case ItemEquipmentSlotType::GlovesRight:
		return equipmentType == ItemEquipmentType::Gloves;
	case ItemEquipmentSlotType::Boots:
		return equipmentType == ItemEquipmentType::Boots;
	case ItemEquipmentSlotType::RingLeft:
	case ItemEquipmentSlotType::RingRight:
		return equipmentType == ItemEquipmentType::Ring;
	case ItemEquipmentSlotType::Amulet:
		return equipmentType == ItemEquipmentType::Amulet;
	}

	return false;
}
