#pragma once
#include "Content/InventroiesModule/models/equipment_slot.h"
#include <QString>
#include <QPixmap>
#include <QUuid>
#include <list>

class EquipmentItem {
public:
	EquipmentItem() {}
	virtual ~EquipmentItem() = default;

	QUuid id;
	EquipmentSlotType slot;
	int level;
	QString type;
	QString subType;
	int width;
	int height;
	int rarity;
	int rows;
	int cols;
	QString iconPath;
	QPixmap icon;
};

class Equipment {
public:
	QUuid id;
	QString name;
	std::list<EquipmentItem> items;
};
