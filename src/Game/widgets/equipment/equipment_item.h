#pragma once
#include <QString>
#include <QPixmap>

class EquipmentSlot;

struct Item {
	enum class Type {
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

	QString id;
	QString name;
	Type type;
	QPixmap icon;
	int rarity = 0; // 0=common, 1=magic, 2=rare, 3=unique

	// Проверка совместимости со слотом
	bool fitsSlot(EquipmentSlot* slot) const;
};
