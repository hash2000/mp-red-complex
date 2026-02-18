#include "Game/services.h"
#include "Game/services/time_service/time_service.h"
#include "Game/services/world_service/world_service.h"
#include "DataLayer/inventory/inventory_data_provider_json_impl.h"
#include "ApplicationLayer/inventory/inventories_service.h"
#include "DataLayer/items/items_data_provider_json_impl.h"
#include "ApplicationLayer/items/items_service.h"
#include <list>

class Services::Private {
public:
	Private(Services* parent)
		: q(parent) {
	}

	Services* q;
	std::unique_ptr<TimeService> timeService;
	std::unique_ptr<WorldService> worldService;
	std::unique_ptr<InventoryDataProvider> inventoryDataProvider;
	std::unique_ptr<InventoriesService> inventoriesService;
	std::unique_ptr<ItemsDataProvider> itemsDataProvider;
	std::unique_ptr<ItemsService> itemsService;
};


Services::Services(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->inventoryDataProvider = std::make_unique<InventoryDataProviderJsonImpl>(resources);
	d->itemsDataProvider = std::make_unique<ItemsDataProviderJsonImpl>(resources);
	d->timeService = std::make_unique<TimeService>();

	d->worldService = std::make_unique<WorldService>();
	connect(this, &Services::save, d->worldService.get(), &WorldService::onSave);
	connect(this, &Services::load, d->worldService.get(), &WorldService::onLoad);

	d->inventoriesService = std::make_unique<InventoriesService>(d->inventoryDataProvider.get());
	connect(this, &Services::save, d->inventoriesService.get(), &InventoriesService::onSave);
	connect(this, &Services::load, d->inventoriesService.get(), &InventoriesService::onLoad);

	d->itemsService = std::make_unique<ItemsService>(d->itemsDataProvider.get());
	d->itemsService->loadEntities();
}

Services::~Services() = default;

void Services::run() {
	d->timeService->start();
}

void Services::postLoadEvent() {
	emit load();
}

void Services::postSaveEvent() {
	emit save();
}

TimeService* Services::timeService() const {
	return d->timeService.get();
}

WorldService* Services::worldService() const {
	return d->worldService.get();
}

InventoriesService* Services::inventoriesService() const {
	return d->inventoriesService.get();
}

ItemsService* Services::itemsService() const {
	return d->itemsService.get();
}
