#include "Game/services.h"
#include "Game/services/time_service/time_service.h"
#include "Game/services/world_service/world_service.h"
#include "DataLayer/inventory/inventory_data_provider_json_impl.h"
#include "DataLayer/inventory/inventory_data_writer_json_impl.h"
#include "DataLayer/items/items_data_provider_json_impl.h"
#include "DataLayer/items/items_data_writer_json_impl.h"
#include "DataLayer/equipment/equipment_data_provider_json_impl.h"
#include "DataLayer/equipment/equipment_data_writer_json_impl.h"
#include "DataLayer/inventories/inventories_data_provider_json_impl.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/inventories_save_manager.h"
#include "ApplicationLayer/items_save_manager.h"
#include "ApplicationLayer/inventory/inventory_store_impl.h"
#include "ApplicationLayer/equipment/equipment_store_impl.h"
#include <list>

class Services::Private {
public:
	Private(Services* parent)
		: q(parent) {
	}

	void loadInventories() {
		std::list<QUuid> items;
		inventoriesDataProvider->loadInventories(items);

		for (const auto& item : items) {
			inventoriesService->addStore(item, std::make_unique<InventoryStoreImpl>(inventoryDataProvider.get(), itemsService.get()));
		}
	}

	void loadEquipments() {
		std::list<QUuid> items;
		inventoriesDataProvider->loadEquipments(items);

		for (const auto& item : items) {
			inventoriesService->addStore(item, std::make_unique<EquipmentStoreImpl>(equipmentDataProvider.get(), itemsService.get()));
		}
	}

	Services* q;
	std::unique_ptr<TimeService> timeService;
	std::unique_ptr<WorldService> worldService;
	std::unique_ptr<InventoryDataProvider> inventoryDataProvider;
	std::unique_ptr<InventoryDataWriter> inventoryDataWriter;
	std::unique_ptr<InventoriesService> inventoriesService;
	std::unique_ptr<InventoriesSaveManager> inventoriesSaveManager;
	std::unique_ptr<ItemsDataProvider> itemsDataProvider;
	std::unique_ptr<ItemsDataWriter> itemsDataWriter;
	std::unique_ptr<ItemsService> itemsService;
	std::unique_ptr<ItemsSaveManager> itemsSaveManager;
	std::unique_ptr<EquipmentDataProvider> equipmentDataProvider;
	std::unique_ptr<EquipmentDataWriter> equipmentDataWriter;
	std::unique_ptr<InventoriesDataProvider> inventoriesDataProvider;
};

Services::Services(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->inventoriesDataProvider = std::make_unique<InventoriesDataProviderJsonImpl>(resources);
	d->inventoryDataProvider = std::make_unique<InventoryDataProviderJsonImpl>(resources);
	d->itemsDataProvider = std::make_unique<ItemsDataProviderJsonImpl>(resources);
	d->equipmentDataProvider = std::make_unique<EquipmentDataProviderJsonImpl>(resources);
	d->inventoryDataWriter = std::make_unique<InventoryDataWriterJsonImpl>(resources);
	d->equipmentDataWriter = std::make_unique<EquipmentDataWriterJsonImpl>(resources);
	d->itemsDataWriter = std::make_unique<ItemsDataWriterJsonImpl>(resources);

	d->timeService = std::make_unique<TimeService>();

	d->worldService = std::make_unique<WorldService>();

	d->itemsService = std::make_unique<ItemsService>(d->itemsDataProvider.get());

	d->inventoriesService = std::make_unique<InventoriesService>(
		d->inventoryDataProvider.get(),
		d->equipmentDataProvider.get(),
		d->itemsService.get());

	d->inventoriesSaveManager = std::make_unique<InventoriesSaveManager>(
		d->inventoriesService.get(),
		d->inventoryDataWriter.get(),
		d->equipmentDataWriter.get());

	d->itemsSaveManager = std::make_unique<ItemsSaveManager>(
		d->itemsService.get(),
		d->itemsDataWriter.get());

	// Подключаем сохранение к сигналу save()
	connect(this, &Services::save,
		d->inventoriesSaveManager.get(), &InventoriesSaveManager::saveAll);
	connect(this, &Services::save,
		d->itemsSaveManager.get(), &ItemsSaveManager::saveAll);
}

Services::~Services() = default;

void Services::run() {
	d->itemsService->loadEntities();
	d->timeService->start();
	d->loadInventories();
	d->loadEquipments();
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

