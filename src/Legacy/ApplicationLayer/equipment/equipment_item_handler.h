#pragma once
#include "DataLayer/items/item.h"
#include "DataLayer/equipment/equipment.h"
#include <QByteArray>
#include <QPoint>
#include <optional>

class EquipmentItemHandler : public EquipmentItem {
public:
	const ItemEntity* entity;
	const Item* item;

public:
	QPoint slotToPosition() const;

	static QPoint convertSlotToPosition(EquipmentSlotType slot);
	static EquipmentSlotType convertPositionToSlot(int col, int row);
};
