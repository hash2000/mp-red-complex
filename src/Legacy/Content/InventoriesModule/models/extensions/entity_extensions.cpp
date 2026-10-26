#include "Content/InventoriesModule/models/extensions/entity_extensions.h"
#include "Content/InventoriesModule/models/item.h"
#include <QDebug>

namespace Inventory::Entities {
ItemResourceType resourceType(const QString& type) {
	if (type == "ore") return ItemResourceType::Ore;
	else if (type == "chemical") return ItemResourceType::Chemical;
	else
		qWarning() << "Load item entity : Undefined resource type:" << type;
	return ItemResourceType::Undefined;
}

ItemType itemType(const QString& type) {
	if (type == "resource") return ItemType::Resource;
	else if (type == "equipment") return ItemType::Equipment;
	else if (type == "component") return ItemType::Component;
	else if (type == "container") return ItemType::Container;
	else if (type == "recipe") return ItemType::Recipe;
	else
		qWarning() << "Load item entity: Undefined type:" << type;
	return ItemType::Undefined;
}

ItemSubType itemSubType(const QString& type) {
	if (type == "head") return ItemSubType::Head;
	else if (type == "body") return ItemSubType::Body;
	else if (type == "weapon") return ItemSubType::Weapon;
	else if (type == "shield") return ItemSubType::Shield;
	else if (type == "gloves") return ItemSubType::Gloves;
	else if (type == "boots") return ItemSubType::Boots;
	else if (type == "ring") return ItemSubType::Ring;
	else if (type == "amulet") return ItemSubType::Amulet;
	else if (type == "resource") return ItemSubType::Resource;
	else if (type == "consumable") return ItemSubType::Consumable;
	else if (type == "backpack") return ItemSubType::Backpack;
	else if (type == "bag") return ItemSubType::Bag;
	else if (type == "root") return ItemSubType::Root;
	else
		qWarning() << "Load item entity: Undefined sub type:" << type;
	return ItemSubType::Undefined;
}

QString resourceTypeToString(ItemResourceType type) {
	switch (type) {
		case ItemResourceType::Ore: return "ore";
		case ItemResourceType::Chemical: return "chemical";
	}
	return QString();
}

QString itemTypeToString(ItemType type) {
	switch (type) {
	case ItemType::Resource: return "resource";
	case ItemType::Equipment: return "equipment";
	case ItemType::Component: return "component";
	case ItemType::Container: return "container";
	case ItemType::Recipe: return "recipe";
	}
	return QString();
}

QString itemSubTypeToString(ItemSubType type) {
	switch (type) {
	case ItemSubType::Head: return "head";
	case ItemSubType::Body: return "body";
	case ItemSubType::Weapon: return "weapon";
	case ItemSubType::Shield: return "shield";
	case ItemSubType::Gloves: return "gloves";
	case ItemSubType::Boots: return "boots";
	case ItemSubType::Ring: return "ring";
	case ItemSubType::Amulet: return "amulet";
	case ItemSubType::Resource: return "resource";
	case ItemSubType::Consumable: return "consumable";
	case ItemSubType::Backpack: return "backpack";
	case ItemSubType::Bag: return "bag";
	case ItemSubType::Root: return "root";
	}
	return QString();
}

QString itemRarityTypeToString(ItemRarityType type) {
	switch (type) {
	case ItemRarityType::Common: return "common";
	case ItemRarityType::Improved: return "improved";
	case ItemRarityType::Stable: return "stable";
	case ItemRarityType::Rare: return "rare";
	case ItemRarityType::Unique: return "unique";
	}
	return QString();
}
}
