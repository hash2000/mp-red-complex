#include "ApplicationLayer/equipment/equipment_item_handler.h"


QPoint EquipmentItemHandler::slotToPosition() const {
	return convertSlotToPosition(slot);
}

QPoint EquipmentItemHandler::convertSlotToPosition(EquipmentSlotType slot) {
	return QPoint(static_cast<qint32>(slot), 0);
}
