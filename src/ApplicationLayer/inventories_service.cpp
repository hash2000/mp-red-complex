#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/items_placement_service.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include "DataLayer/items/items_data_provider.h"
#include <QDebug>
#include <map>

class InventoriesService::Private {
public:
	Private(InventoriesService* parent)
		: q(parent) {
	}

	InventoriesService* q;
	InventoriesDataProvider* inventoriesDataProvider;
	ItemsService* itemsService;
};

InventoriesService::InventoriesService(
	ItemsService* itemsService,
	QObject* parent)
: d(std::make_unique<Private>(this)) {
	d->itemsService = itemsService;
}

InventoriesService::~InventoriesService() = default;

ItemPlacementService* InventoriesService::placementService(const QUuid& id, bool loadIfNotExists) const {
	return nullptr;
}

bool InventoriesService::moveItem(const ItemMimeData& item, int col, int row, const QUuid& fromId, const QUuid& toId) {
	auto fromService = placementService(fromId, false);
	auto toService = placementService(toId, false);

	if (!fromService || !toService) {
		qWarning() << "moveItem: service not found"
			<< "from:" << fromId << "to:" << toId;
		return false;
	}

	// Получаем исходный предмет из инвентаря
	const auto sourceItem = fromService->itemDataById(item.id);
	if (sourceItem.owner == static_cast<qint32>(ItemOwner::Unknown)) {
		qWarning() << "moveItem: source item not found" << item.id;
		return false;
	}

	// Проверяем, перемещаем ли мы весь предмет или часть пачки
	bool isFullStackMove = (sourceItem.count == item.count);
	if (isFullStackMove) {
		// Перемещаем весь предмет с сохранением ID (без создания копии)

		int targetCol = col;
		int targetRow = row;

		if (!toService->canPlaceItem(item, col, row, false)) {

			// Находим свободное место в целевом инвентаре
			auto freeSpace = toService->findFreeSpace(item, false);
			if (!freeSpace.has_value()) {
				qWarning() << "moveItem: no free space in target inventory for item" << item.id;
				return false;
			}

			targetCol = freeSpace->x();
			targetRow = freeSpace->y();
		}

		// Размещаем предмет в целевом инвентаре с сохранением ID
		ItemMimeData toItemData = item;
		toItemData.x = targetCol;
		toItemData.y = targetRow;

		// Используем метод transferItem для размещения с сохранением ID
		if (!toService->placeItem(toItemData)) {
			qWarning() << "moveItem: failed to place item in target inventory" << item.id;
			return false;
		}

		// Удаляем из исходного инвентаря
		fromService->removeItem(item);

		// Отправляем событие о перемещении
		emit itemMoved(item, fromId, toId);
		return true;
	}

	// Для частичного перемещения создаём новую копию (старое поведение)
	ItemMimeData changedItem = item;
	if (!fromService->removeItemsFromStack(changedItem)) {
		return false;
	}

	if (changedItem.count == 0) {
		emit itemMoved(item, fromId, toId);
		return true;
	}

	changedItem.x = col;
	changedItem.y = row;

	if (!toService->placeItem(changedItem)) {
		return false;
	}

	emit itemMoved(item, fromId, toId);
	return true;
}

