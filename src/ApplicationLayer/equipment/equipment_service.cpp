#include "ApplicationLayer/equipment/equipment_service.h"
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include <QDebug>

class EquipmentService::Private {
public:
	Private(EquipmentService* parent)
		: q(parent) {
	}

	EquipmentService* q;
	ItemsService* itemsService;
	QString equipmentId;
	std::map<EquipmentSlotType, std::unique_ptr<EquipmentItemHandler>> items;
};

EquipmentService::EquipmentService(ItemsService* itemsService, QObject* parent)
	: d(std::make_unique<Private>(this)) {
	d->itemsService = itemsService;
}

EquipmentService::~EquipmentService() = default;

const EquipmentItemHandler* EquipmentService::itemBySlot(EquipmentSlotType slot) const {
	const auto& it = d->items.find(slot);
	if (it == d->items.end()) {
		return nullptr;
	}

	return it->second.get();
}

bool EquipmentService::load(const Equipment& equipment) {
	d->equipmentId = equipment.id;

	for (const auto& it : equipment.items) {
		const auto item = d->itemsService->itemById(it.id);
		if (!item) {
			qWarning() << "EquipmentService::load: equipment" << d->equipmentId << ". undefined item:" << it.id;
			continue;
		}

		auto eq = std::make_unique<EquipmentItemHandler>();
		eq->id = it.id;
		eq->slot = it.slot;
		eq->entity = item->entity;

		if (d->items.contains(eq->slot)) {
			qWarning() << "EquipmentService::load: equipment" << d->equipmentId <<
				". item:" << it.id << "current slot allready equiped";
			continue;
		}

		d->items.emplace(eq->slot, std::move(eq));
	}

	return true;
}

QString EquipmentService::equipmentId() const {
	return d->equipmentId;
}

bool EquipmentService::canAcceptItem(const ItemMimeData& item, EquipmentSlotType slot) const {
	const auto equipmentType = static_cast<ItemEquipmentType>(item.equipmentType);

	switch (slot) {
	case EquipmentSlotType::Head:
		return equipmentType == ItemEquipmentType::Head;
	case EquipmentSlotType::Body:
		return equipmentType == ItemEquipmentType::Body;
	case EquipmentSlotType::WeaponLeft:
		return equipmentType == ItemEquipmentType::Weapon || equipmentType == ItemEquipmentType::Shield;
	case EquipmentSlotType::WeaponRight:
		return equipmentType == ItemEquipmentType::Weapon;
	case EquipmentSlotType::GlovesLeft:
	case EquipmentSlotType::GlovesRight:
		return equipmentType == ItemEquipmentType::Gloves;
	case EquipmentSlotType::Boots:
		return equipmentType == ItemEquipmentType::Boots;
	case EquipmentSlotType::RingLeft:
	case EquipmentSlotType::RingRight:
		return equipmentType == ItemEquipmentType::Ring;
	case EquipmentSlotType::Amulet:
		return equipmentType == ItemEquipmentType::Amulet;
	case EquipmentSlotType::Backpack:
		return equipmentType == ItemEquipmentType::Backpack;
	case EquipmentSlotType::Bag1:
	case EquipmentSlotType::Bag2:
		return equipmentType == ItemEquipmentType::Bag;
	}

	return false;
}


const EquipmentItemHandler* EquipmentService::equipItem(const ItemMimeData& item, EquipmentSlotType slot, const QString& inventoryId) {
	// берём из инвентаря
	const auto itemPtr = d->itemsService->itemById(item.id);
	if (!itemPtr) {
		return nullptr;
	}

	if (d->items.contains(slot)) {
		// сначала нужно освободить слот
		return NULL;
	}

	auto eq = std::make_unique<EquipmentItemHandler>();
	eq->entity = itemPtr->entity;
	eq->id = itemPtr->id;
	eq->slot = slot;

	const auto &emplaceResult = d->items.emplace(slot, std::move(eq));
	
	emit itemEquipped(*emplaceResult.first->second, slot, inventoryId);
	return emplaceResult.first->second.get();
}

bool EquipmentService::unequipItem(const ItemMimeData& item, EquipmentSlotType slot) {
	const auto itemPtr = d->itemsService->itemById(item.id);
	if (!itemPtr) {
		return false;
	}

	const auto& it = d->items.find(slot);
	if (it == d->items.end()) {
		return false;
	}

	const auto eqItem = std::move(it->second);

	d->items.erase(slot);

	emit itemUnequipped(*eqItem, slot);
	return true;
}
