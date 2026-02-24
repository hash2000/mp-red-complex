#include "ApplicationLayer/inventory/inventories_service.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/inventory/inventory_item_mime_data.h"
#include "DataLayer/inventory/inventory_data_provider.h"
#include "DataLayer/inventory/inventory_item.h"
#include <QDebug>
#include <map>

class InventoriesService::Private {
public:
	Private(InventoriesService* parent)
		: q(parent) {
	}

	InventoriesService* q;
	InventoryDataProvider* dataProvider;
	ItemsService* itemsService;
	std::map<QUuid, std::unique_ptr<InventoryService>> inventories;
};

InventoriesService::InventoriesService(InventoryDataProvider* dataProvider, ItemsService* itemsService, QObject* parent)
	: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
	d->itemsService = itemsService;
}

InventoriesService::~InventoriesService() = default;


InventoryService* InventoriesService::inventoryService(const QUuid& id, bool loadIfNotExists) const {
	if (!d->inventories.contains(id)) {
		if (!loadIfNotExists) {
			return nullptr;
		}

		Inventory inventoryData;
		if (!d->dataProvider->loadInventory(id, inventoryData)) {
			return nullptr;
		}

		auto inventory = std::make_unique<InventoryService>(d->itemsService);
		if (!inventory->load(inventoryData)) {
			return nullptr;
		}

		d->inventories[id] = std::move(inventory);
		qDebug() << "Load inventory" << id;
	}

	return d->inventories[id].get();
}

bool InventoriesService::crossInventoryMove(const InventoryItemMimeData& item, int col, int row, const QUuid& fromInventoryId, const QUuid& toInventoryId) {
	auto fromService = inventoryService(fromInventoryId, false);
	auto toService = inventoryService(toInventoryId, false);

	if (!fromService || !toService) {
		qWarning() << "crossInventoryMove: inventory service not found"
			<< "from:" << fromInventoryId << "to:" << toInventoryId;
		return false;
	}

	// в предыдущем инвентаре изменяем количество элементов в стопке, если она есть
	// если элементов станет 0, то из этого инвентаря элемент удалится

	InventoryItemMimeData changedItem = item;
	if (!fromService->changeItemsCount(changedItem)) {
		return false;
	}

	if (changedItem.count == 0) {
		return true;
	}

	InventoryItemMimeData toItem = item;
	toItem.count = changedItem.count;
	toItem.x = col;
	toItem.y = row;

	// во втором инвентаре нужно или добавить к соответствующей пачке
	// или положить всю пачку в пустую ячейку 
	toService->applyItem(toItem);

	return true;
}

void InventoriesService::onInventoryClosed(const QUuid& id) {
	const auto & result = d->inventories.erase(id);
	if (result != 0) {
		qDebug() << "Unload inventory" << id;
	}
}

void InventoriesService::onSave() {

}

void InventoriesService::onLoad() {

}
