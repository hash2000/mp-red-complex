#include "Content/InventroiesModule/services/equipment_service.h"
#include "Content/InventroiesModule/services/items_service.h"
#include "Content/InventroiesModule/models/equipment_item_handler.h"
#include "Content/InventroiesModule/models/item_mime_data.h"
#include <QDebug>
#include <map>

class EquipmentService::Private {
public:
	Private(EquipmentService* parent)
		: q(parent) {
	}

	EquipmentService* q;
	ItemsService* itemsService;
	QUuid equipmentId;
	std::map<EquipmentSlotType, std::unique_ptr<EquipmentItemHandler>> items;
};

EquipmentService::EquipmentService(ItemsService* itemsService, QObject* parent)
	: IItemPlacementService(parent)
	, d(std::make_unique<Private>(this)) {
	d->itemsService = itemsService;
}

EquipmentService::~EquipmentService() = default;

const EquipmentItemHandler* EquipmentService::itemBySlot(EquipmentSlotType slot) const {

	return nullptr;
}

EquipmentService::ItemsMap EquipmentService::items() const {
	return d->items;
}

bool EquipmentService::load(const Equipment& equipment) {

	return true;
}

QUuid EquipmentService::placementId() const {
	return d->equipmentId;
}

QString EquipmentService::placementName() const {
	return QString("Equipment");
}

int EquipmentService::canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const {
	const auto equipmentType = static_cast<ItemSubType>(item.subType);
	const auto slot = EquipmentItemHandler::convertPositionToSlot(col, row);

	switch (slot) {
	case EquipmentSlotType::Head:
		return equipmentType == ItemSubType::Head ? 1 : 0;
	case EquipmentSlotType::Body:
		return equipmentType == ItemSubType::Body ? 1 : 0;
	case EquipmentSlotType::WeaponLeft:
		return (equipmentType == ItemSubType::Weapon || equipmentType == ItemSubType::Shield) ? 1 : 0;
	case EquipmentSlotType::WeaponRight:
		return equipmentType == ItemSubType::Weapon ? 1 : 0;
	case EquipmentSlotType::Gloves:
		return equipmentType == ItemSubType::Gloves ? 1 : 0;
	case EquipmentSlotType::Boots:
		return equipmentType == ItemSubType::Boots ? 1 : 0;
	case EquipmentSlotType::Ring1:
	case EquipmentSlotType::Ring2:
	case EquipmentSlotType::Ring3:
		return equipmentType == ItemSubType::Ring ? 1 : 0;
	case EquipmentSlotType::Amulet:
		return equipmentType == ItemSubType::Amulet ? 1 : 0;
	case EquipmentSlotType::Backpack:
		return equipmentType == ItemSubType::Backpack ? 1 : 0;
	case EquipmentSlotType::Bag1:
	case EquipmentSlotType::Bag2:
		return equipmentType == ItemSubType::Bag ? 1 : 0;
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
	return static_cast<qint32>(ItemSubType::LastSlot);
}

ItemMimeData EquipmentService::EquipmentService::itemDataById(const QUuid& id) const {

	return ItemMimeData();
}

bool EquipmentService::containsItem(const ItemMimeData& item) const {
	return true;
}

bool EquipmentService::moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) {
	Q_UNUSED(item);
	Q_UNUSED(newCol);
	Q_UNUSED(newRow);
	Q_UNUSED(checkItemPlace);
	return false;
}

void EquipmentService::removeItem(const ItemMimeData& item) {

}

void EquipmentService::clear() {
	d->items.clear();
}

bool EquipmentService::placeItem(const ItemMimeData& item) {
	return true;
}

bool EquipmentService::removeItemsFromStack(const ItemMimeData& item) {
	return true;
}

const EquipmentItemHandler* EquipmentService::equipItem(const ItemMimeData& item, EquipmentSlotType slot, const QUuid& inventoryId) {
	return nullptr;
}

bool EquipmentService::unequipItem(const ItemMimeData& item, EquipmentSlotType slot) {

	return true;
}

void EquipmentService::clearResourcesPermissions() {

}

void EquipmentService::addResourcesPermissions(
	const std::list<ItemResourceType>& all,
	const std::list<ItemResourceType>& any) {

}
