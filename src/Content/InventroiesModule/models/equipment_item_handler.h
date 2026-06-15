#pragma once
#include "Content/InventroiesModule/models/item.h"
#include "Content/InventroiesModule/models/equipment.h"
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
