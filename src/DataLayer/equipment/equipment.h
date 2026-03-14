#pragma once
#include "DataLayer/equipment/equipment_slot.h"
#include <QString>
#include <list>

class EquipmentItem {
public:
	QString id;
	EquipmentSlotType slot;
};

class Equipment {
public:
	QString id;
	std::list<EquipmentItem> items;
};
