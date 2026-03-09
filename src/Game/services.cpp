#include "Game/services.h"
#include "Game/services/time_service/time_service.h"
#include "Game/services/world_service/world_service.h"
#include "DataLayer/inventory/inventory_data_provider_json_impl.h"
#include "DataLayer/items/items_data_provider_json_impl.h"
#include "DataLayer/equipment/equipment_data_provider_json_impl.h"
#include "DataLayer/inventories/inventories_data_provider_json_impl.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventories_service.h"
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
	std::unique_ptr<EquipmentDataProvider> equipmentDataProvider;
	std::unique_ptr<InventoriesDataProvider> inventoriesDataProvider;
};

Services::Services(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->inventoriesDataProvider = std::make_unique<InventoriesDataProviderJsonImpl>(resources);
	d->inventoryDataProvider = std::make_unique<InventoryDataProviderJsonImpl>(resources);
	d->itemsDataProvider = std::make_unique<ItemsDataProviderJsonImpl>(resources);
	d->equipmentDataProvider = std::make_unique<EquipmentDataProviderJsonImpl>(resources);

	d->timeService = std::make_unique<TimeService>();

	d->worldService = std::make_unique<WorldService>();

	d->itemsService = std::make_unique<ItemsService>(d->itemsDataProvider.get());
	
	d->inventoriesService = std::make_unique<InventoriesService>(
		d->inventoriesDataProvider.get(),
		d->inventoryDataProvider.get(),
		d->equipmentDataProvider.get(),
		d->itemsService.get());
}

Services::~Services() = default;

void Services::run() {
	d->itemsService->loadEntities();
	d->timeService->start();
	d->inventoriesService->load();
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

ItemsService* Services::itemsService() const {
	return d->itemsService.get();
}

InventoriesService* Services::inventoriesService() const {
	return d->inventoriesService.get();
}

