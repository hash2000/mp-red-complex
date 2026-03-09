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

QString EquipmentService::placementId() const {
	return d->equipmentId;
}

int EquipmentService::canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const {
	const auto equipmentType = static_cast<ItemEquipmentType>(item.equipmentType);
	const auto slot = EquipmentItemHandler::convertPositionToSlot(col, row);

	switch (slot) {
	case EquipmentSlotType::Head:
		return equipmentType == ItemEquipmentType::Head ? 1 : 0;
	case EquipmentSlotType::Body:
		return equipmentType == ItemEquipmentType::Body ? 1 : 0;
	case EquipmentSlotType::WeaponLeft:
		return (equipmentType == ItemEquipmentType::Weapon || equipmentType == ItemEquipmentType::Shield) ? 1 : 0;
	case EquipmentSlotType::WeaponRight:
		return equipmentType == ItemEquipmentType::Weapon ? 1 : 0;
	case EquipmentSlotType::GlovesLeft:
	case EquipmentSlotType::GlovesRight:
		return equipmentType == ItemEquipmentType::Gloves ? 1 : 0;
	case EquipmentSlotType::Boots:
		return equipmentType == ItemEquipmentType::Boots ? 1 : 0;
	case EquipmentSlotType::RingLeft:
	case EquipmentSlotType::RingRight:
		return equipmentType == ItemEquipmentType::Ring ? 1 : 0;
	case EquipmentSlotType::Amulet:
		return equipmentType == ItemEquipmentType::Amulet ? 1 : 0;
	case EquipmentSlotType::Backpack:
		return equipmentType == ItemEquipmentType::Backpack ? 1 : 0;
	case EquipmentSlotType::Bag1:
	case EquipmentSlotType::Bag2:
		return equipmentType == ItemEquipmentType::Bag ? 1 : 0;
	}

	return 0;
}

std::optional<QPoint> EquipmentService::findFreeSpace(const ItemMimeData& item, bool checkItemPlace) const {
	return std::nullopt;
}

int EquipmentService::rows() const {
	return 1;
}

int EquipmentService::cols() const {
	return static_cast<qint32>(ItemEquipmentType::LastSlot);
}

bool EquipmentService::containsItem(const ItemMimeData& item) const {
	const auto slot = EquipmentItemHandler::convertPositionToSlot(item.x, item.y);
	const auto it = d->items.find(slot);
	if (it == d->items.end()) {
		return false;
	}
	return it->second->id == item.id;
}

bool EquipmentService::moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) {
	Q_UNUSED(item);
	Q_UNUSED(newCol);
	Q_UNUSED(newRow);
	Q_UNUSED(checkItemPlace);
	return false;
}
void EquipmentService::removeItem(const ItemMimeData& item) {
	const auto slot = EquipmentItemHandler::convertPositionToSlot(item.x, item.y);
	const auto it = d->items.find(slot);
	if (it != d->items.end()) {
		const auto pos = it->second->slotToPosition();
		emit removeItemEvent(item, pos.y(), pos.x());
		d->items.erase(it);
	}
}

void EquipmentService::clear() {
	d->items.clear();
}

bool EquipmentService::applyDublicateFromItem(const ItemMimeData& item) {
	if (item.equipmentType == 0) {
		return false;
	}

	const auto slot = static_cast<EquipmentSlotType>(item.equipmentType);
	if (d->items.contains(slot)) {
		return false;
	}

	const auto itemPtr = d->itemsService->itemById(item.id);
	if (!itemPtr) {
		return false;
	}

	auto eq = std::make_unique<EquipmentItemHandler>();
	eq->entity = itemPtr->entity;
	eq->id = itemPtr->id;
	eq->slot = slot;

	d->items.emplace(slot, std::move(eq));
	emit itemEquipped(*d->items.at(slot), slot, placementId());
	return true;
}

bool EquipmentService::removeItemsFromStack(const ItemMimeData& item) {
	const auto slot = EquipmentItemHandler::convertPositionToSlot(item.y, item.x);
	const auto it = d->items.find(slot);
	if (it == d->items.end()) {
		return false;
	}

	emit itemUnequipped(*it->second, slot);
	d->items.erase(it);
	return true;
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
