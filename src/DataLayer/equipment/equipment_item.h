#pragma once
#include <QString>
#include <QPixmap>

enum class EquipmentItemType {
	Head,
	Body,
	Weapon,
	Shield,
	Gloves,
	Boots,
	Ring,
	Amulet,
	Consumable,
	Resource,
};

enum class EquipmentItemRarityType {
	Common,
	Improved,
	Stable,
	Rare,
	Unique,
};

struct EquipmentItem {
	QString id;
	QString name;
	EquipmentItemType type;
	QPixmap icon;
	EquipmentItemRarityType rarity = EquipmentItemRarityType::Common;
};
