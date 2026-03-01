#pragma once
#include "DataLayer/items/item.h"
#include "DataLayer/equipment/equipment.h"
#include <QByteArray>
#include <optional>

class EquipmentItemHandler : public EquipmentItem {
public:
	const ItemEntity* entity;
};
