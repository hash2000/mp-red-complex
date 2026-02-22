#include "ApplicationLayer/inventory/inventories_service.h"
#include "ApplicationLayer/inventory/inventory_service.h"
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
	// 1. Получаем сервисы инвентарей
	auto fromService = inventoryService(fromInventoryId, false);
	auto toService = inventoryService(toInventoryId, false);

	if (!fromService || !toService) {
		qWarning() << "crossInventoryMove: inventory service not found"
			<< "from:" << fromInventoryId << "to:" << toInventoryId;
		return false;
	}

	// 2. Находим полный предмет в ИСХОДНОМ инвентаре по ID
	auto itemPtr = fromService->itemById(item.id);
	if (!itemPtr) {
		qWarning() << "crossInventoryMove: item not found in source inventory" << item.id;
		return false;
	}

	//// 3. Проверяем возможность размещения в ЦЕЛЕВОМ инвентаре
	//const auto count = toService->canPlaceItem(item, col, row, false);
	//if (count == 0) {
	//	return false;
	//}

	//std::shared_ptr<InventoryItem> itemToMove;

	//{
	//	// Блок для ограничения времени жизни итератора
	//	auto& fromItems = fromService->inventory()->items;
	//	auto it = fromItems.find(item.id);
	//	if (it == fromItems.end()) {
	//		qWarning() << "crossInventoryMove: item vanished during move" << item.id;
	//		return false;
	//	}

	//	// Сохраняем владение перед удалением
	//	itemToMove = it->second;

	//	const auto remains = qMax(itemToMove->count - item.count, 0);
	//	if (remains == 0) {
	//		fromService->removeItem(item);
	//	}
	//	else {
	//		// стопка выведена не полностью, поэтому исходную стопку оставляем
	//		// и создаём новую
	//		itemToMove->count = remains;
	//		itemToMove = itemToMove->duplicate(true);
	//		itemToMove->count = item.count;
	//	}
	//}

	//itemToMove->x = col;
	//itemToMove->y = row;

	//// 5. Размещаем в целевом инвентаре
	//toService->inventory()->items.emplace(itemToMove->id, itemToMove);

	//bool placed = toService->placeItem(*itemToMove);

	//if (!placed) {
	//	qWarning() << "crossInventoryMove: failed to place item in target inventory" << item.id;
	//	// Возвращаем предмет обратно в исходный инвентарь при ошибке
	//	itemToMove->x = item.x; // Восстанавливаем оригинальные координаты
	//	itemToMove->y = item.y;

	//	fromService->inventory()->items.emplace(itemToMove->id, itemToMove);
	//	if (!fromService->placeItem(*itemToMove)) {
	//		qWarning() << "crossInventoryMove: failed to return back item in source inventory" << item.id;
	//	}

	//	return false;
	//}

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
