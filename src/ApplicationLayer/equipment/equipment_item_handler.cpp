#include "ApplicationLayer/equipment/equipment_item_handler.h"


QPoint EquipmentItemHandler::slotToPosition() const {
	return convertSlotToPosition(slot);
}

QPoint EquipmentItemHandler::convertSlotToPosition(EquipmentSlotType slot) {
	return QPoint(static_cast<qint32>(slot), 0);
}

EquipmentSlotType EquipmentItemHandler::convertPositionToSlot(int col, int row) {
	return static_cast<EquipmentSlotType>(row);
}
