#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/items_placement_service.h"
#include "ApplicationLayer/inventory_loader.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include <QDebug>

class InventoriesService::Private {
public:
	Private(InventoriesService* parent)
		: q(parent) {
	}

	InventoriesService* q;
	std::map<QUuid, std::unique_ptr<InventoryLoader>> loaders;
	std::map<QUuid, std::unique_ptr<ItemPlacementService>> services;
};

InventoriesService::InventoriesService(QObject* parent)
	: d(std::make_unique<Private>(this)) {
}

InventoriesService::~InventoriesService() = default;

ItemPlacementService* InventoriesService::placementService(const QUuid& id, bool loadIfNotExists) const {
	const auto it = d->services.find(id);
	if (it != d->services.end()) {
		return it->second.get();
	}

	if (!loadIfNotExists) {
		qWarning() << "Inventory not found" << id.toString();
		return nullptr;
	}

	const auto& itl = d->loaders.find(id);
	if (itl == d->loaders.end()) {
		qWarning() << "Inventory loader not found" << id.toString();
		return nullptr;
	}

	auto service = itl->second->load(id);
	const auto insertResult = d->services.emplace(id, std::move(service));
	return insertResult.first->second.get();
}

bool InventoriesService::applyLoader(const QUuid& id, std::unique_ptr<InventoryLoader> loader) {
	const auto& result = d->loaders.emplace(id, std::move(loader));
	return result.second;
}

bool InventoriesService::moveItem(const ItemMimeData& item, int col, int row, const QUuid& fromId, const QUuid& toId) {
	auto fromService = placementService(fromId, false);
	auto toService = placementService(toId, false);

	if (!fromService || !toService) {
		qWarning() << "moveItem: service not found"
			<< "from:" << fromId << "to:" << toId;
		return false;
	}

	ItemMimeData changedItem = item;
	if (!fromService->removeItemsFromStack(changedItem)) {
		return false;
	}

	if (changedItem.count == 0) {
		emit itemMoved(item, fromId, toId);
		return true;
	}

	ItemMimeData toItem = item;
	toItem.count = changedItem.count;
	toItem.x = col;
	toItem.y = row;

	if (!toService->applyDublicateFromItem(toItem)) {
		fromService->applyDublicateFromItem(item);
		return false;
	}

	emit itemMoved(item, fromId, toId);
	return true;
}
