#include "Content/InventroiesModule/services/inventory_service.h"
#include "Content/InventroiesModule/services/items_service.h"
#include "Content/InventroiesModule/models/inventory_item_handler.h"
#include "Content/InventroiesModule/models/item_mime_data.h"
#include "Content/InventroiesModule/models/item.h"
#include <QHash>
#include <QDebug>
#include <vector>
#include <algorithm>

struct InventoryViewCell {
	bool occupied = false;
	InventoryItemHandler* item = nullptr;
};


class InventoryService::Private {
public:
	Private(InventoryService* parent) : q(parent) { }
	InventoryService* q;

	ItemsService* itemsService;
	QUuid placementId;
	QString inventoryName;
	int rows;
	int cols;
	std::map<QUuid, std::unique_ptr<InventoryItemHandler>> inventoryItems;
	std::vector<std::vector<std::unique_ptr<InventoryViewCell>>> cells;
	QHash<QPoint, InventoryItemHandler*> items;
	ItemContainerPermissions permissions;
};

InventoryService::InventoryService(ItemsService* itemsService, QObject* parent)
: IItemPlacementService(parent)
, d(std::make_unique<Private>(this)) {
	d->itemsService = itemsService;
}

InventoryService::~InventoryService() = default;

bool InventoryService::load(const Inventory& inventory) {
	return true;
}

QUuid InventoryService::placementId() const {
	return d->placementId;
}

QString InventoryService::placementName() const {
	return d->inventoryName;
}

int InventoryService::rows() const {
	return d->rows;
}

int InventoryService::cols() const {
	return d->cols;
}

const InventoryItemHandler* InventoryService::itemAt(int col, int row) const {
	return nullptr;
}

const InventoryItemHandler* InventoryService::itemById(const QUuid& id) const {
	return nullptr;
}

ItemMimeData InventoryService::itemDataById(const QUuid& id) const {
	return ItemMimeData();
}

bool InventoryService::placeItem(const ItemMimeData& item) {
	return true;
}

int InventoryService::canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const {

	return 1;
}

std::optional<QPoint> InventoryService::findFreeSpace(const ItemMimeData& item, bool checkItemPlace) const {
	return std::nullopt;
}

void InventoryService::clear() {


	d->items.clear();
}

void InventoryService::removeItem(const ItemMimeData& item) {

}

bool InventoryService::moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) {

	return true;
}

bool InventoryService::containsItem(const ItemMimeData& item) const {

	return true;
}


bool InventoryService::removeItemsFromStack(const ItemMimeData& item) {


	return true;
}

void InventoryService::clearResourcesPermissions() {
	d->permissions.resources.all.clear();
	d->permissions.resources.any.clear();
}

void InventoryService::addResourcesPermissions(
	const std::list<ItemResourceType>& all,
	const std::list<ItemResourceType>& any) {
	d->permissions.resources.all.insert(
		d->permissions.resources.all.end(),
		all.begin(),
		all.end());
	d->permissions.resources.any.insert(
		d->permissions.resources.any.end(),
		any.begin(),
		any.end());
}
