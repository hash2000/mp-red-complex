#include "Content/InventroiesModule/services/inventories_service.h"
#include "Content/InventroiesModule/services/items_placement_service.h"
#include "Content/InventroiesModule/services/inventory_service.h"
#include "Content/InventroiesModule/services/items_service.h"
#include "Content/InventroiesModule/services/equipment_service.h"
#include "Content/InventroiesModule/models/inventory_item_handler.h"
#include "Content/InventroiesModule/models/item_mime_data.h"
#include <QDebug>
#include <map>
#include <vector>

class InventoriesService::Private {
public:
	Private(InventoriesService* parent)
		: q(parent) {
	}

	InventoriesService* q;
	ItemsService* itemsService;

	// Кэш загруженных сервисов
	mutable std::map<QUuid, IItemPlacementService*> loadedServices;
};

InventoriesService::InventoriesService(
	ItemsService* itemsService,
	QObject* parent)
	: d(std::make_unique<Private>(this)) {
	d->itemsService = itemsService;
}

InventoriesService::~InventoriesService() {
	for (auto& [uid, inv] : d->loadedServices) {
		inv->deleteLater();
	}

	d->loadedServices.clear();
}

IItemPlacementService* InventoriesService::placementService(const QUuid& id, bool loadIfNotExists) const {


	return nullptr;
}

bool InventoriesService::moveItem(const ItemMimeData& item, int col, int row, const QUuid& fromId, const QUuid& toId) {

	return true;
}

std::vector<QUuid> InventoriesService::loadedPlacementIds() const {

	return std::vector<QUuid>();
}
