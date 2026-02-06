#include "Game/widgets/equipment/equipment_item.h"
#include "Game/widgets/equipment/equipment_slot.h"

bool Item::fitsSlot(EquipmentSlot* slot) const {
	if (!slot) {
		return false;
	}

	switch (type) {
	case Type::Head: return slot->slotType() == EquipmentSlot::SlotType::Head;
	case Type::Body: return slot->slotType() == EquipmentSlot::SlotType::Body;
	case Type::Weapon:
		return slot->slotType() == EquipmentSlot::SlotType::WeaponLeft ||
			slot->slotType() == EquipmentSlot::SlotType::WeaponRight;
	case Type::Shield:
		return slot->slotType() == EquipmentSlot::SlotType::WeaponLeft; // Щит только в левой
	case Type::Gloves:
		return slot->slotType() == EquipmentSlot::SlotType::GlovesLeft ||
			slot->slotType() == EquipmentSlot::SlotType::GlovesRight;
	case Type::Boots: return slot->slotType() == EquipmentSlot::SlotType::Boots;
	case Type::Ring:
		return slot->slotType() == EquipmentSlot::SlotType::RingLeft ||
			slot->slotType() == EquipmentSlot::SlotType::RingRight;
	case Type::Amulet: return slot->slotType() == EquipmentSlot::SlotType::Amulet;
	default: return false;
	}
}
