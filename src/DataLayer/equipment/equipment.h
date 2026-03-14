#pragma once
#include "DataLayer/equipment/equipment_slot.h"
#include <QString>
#include <QUuid>
#include <list>

class EquipmentItem {
public:
	QUuid id;
	EquipmentSlotType slot;
};

class Equipment {
public:
	QUuid id;
	std::list<EquipmentItem> items;
};
