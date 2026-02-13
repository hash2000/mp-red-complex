#include "Game/services/inventory/inventories_service.h"
#include "Game/services/inventory/inventory_service.h"
#include <map>

class InventoriesService::Private {
public:
	Private(InventoriesService* parent)
	: q(parent) {
	}

	InventoriesService* q;
	InventoryDataProvider* dataProvider;
	std::map<QUuid, std::unique_ptr<InventoryService>> inventories;
};

InventoriesService::InventoriesService(InventoryDataProvider* dataProvider, QObject* parent)
: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
}

InventoriesService::~InventoriesService() = default;

InventoryService* InventoriesService::inventoryService(const QUuid& id, bool loadIfNotExists) const {
	if (!d->inventories.contains(id)) {
		if (!loadIfNotExists) {
			return nullptr;
		}

		auto inventory = d->dataProvider->loadInventory(id);
		d->inventories[id] = std::make_unique<InventoryService>(inventory);
	}

	return d->inventories[id].get();
}

bool InventoriesService::crossInventoryMove(const InventoryHandler& item, int col, int row, const QUuid& fromInventoryId, const QUuid& toInventoryId) {
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

	// 3. Проверяем возможность размещения в ЦЕЛЕВОМ инвентаре
	if (!toService->canPlaceItem(item, col, row, false)) {
		return false;
	}

	std::shared_ptr<InventoryItem> itemToMove;

	{
		// Блок для ограничения времени жизни итератора
		auto& fromItems = fromService->inventory()->items;
		auto it = fromItems.find(item.id);
		if (it == fromItems.end()) {
			qWarning() << "crossInventoryMove: item vanished during move" << item.id;
			return false;
		}

		// Сохраняем владение перед удалением
		itemToMove = it->second;

		// Удаляем из исходного инвентаря (освобождает только карту, не сам предмет благодаря itemToMove)
		fromService->removeItem(item);

		// Обновляем координаты предмета для нового инвентаря
		itemToMove->x = col;
		itemToMove->y = row;
	}

	// 5. Размещаем в целевом инвентаре
	toService->inventory()->items.emplace(itemToMove->id, itemToMove);

	bool placed = toService->placeItem(*itemToMove);

	if (!placed) {
		qWarning() << "crossInventoryMove: failed to place item in target inventory" << item.id;
		// Возвращаем предмет обратно в исходный инвентарь при ошибке
		itemToMove->x = item.x; // Восстанавливаем оригинальные координаты
		itemToMove->y = item.y;

		fromService->inventory()->items.emplace(itemToMove->id, itemToMove);
		if (!fromService->placeItem(*itemToMove)) {
			qWarning() << "crossInventoryMove: failed to return back item in source inventory" << item.id;
		}

		return false;
	}

	return true;

}


void InventoriesService::onSave() {

}

void InventoriesService::onLoad() {

}
