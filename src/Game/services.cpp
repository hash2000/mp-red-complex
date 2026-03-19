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
#include "DataLayer/items/item_repository_json_impl.h"
#include "DataLayer/inventory/inventory_repository_json_impl.h"
#include "DataLayer/equipment/equipment_repository_json_impl.h"
#include "DataLayer/users/users_data_provider_json_impl.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/inventory_loader.h"
#include "ApplicationLayer/inventories_save_manager.h"
#include "ApplicationLayer/items_save_manager.h"
#include "ApplicationLayer/users/users_service.h"
#include <list>

class Services::Private {
public:
	Private(Services* parent)
		: q(parent) {
	}

	Services* q;
	std::unique_ptr<TimeService> timeService;
	std::unique_ptr<WorldService> worldService;

	// Data Providers (остаются для обратной совместимости и writer'ов)
	std::unique_ptr<InventoryDataProvider> inventoryDataProvider;
	std::unique_ptr<InventoryDataWriter> inventoryDataWriter;
	std::unique_ptr<ItemsDataProvider> itemsDataProvider;
	std::unique_ptr<ItemsDataWriter> itemsDataWriter;
	std::unique_ptr<EquipmentDataProvider> equipmentDataProvider;
	std::unique_ptr<EquipmentDataWriter> equipmentDataWriter;
	std::unique_ptr<InventoriesDataProvider> inventoriesDataProvider;

	// Repositories (новый слой абстракции)
	std::shared_ptr<IItemRepository> itemRepository;
	std::shared_ptr<IInventoryRepository> inventoryRepository;
	std::shared_ptr<IEquipmentRepository> equipmentRepository;

	// Services
	std::unique_ptr<ItemsService> itemsService;
	std::unique_ptr<InventoriesService> inventoriesService;
	std::unique_ptr<InventoryLoader> inventoryLoader;
	std::unique_ptr<InventoriesSaveManager> inventoriesSaveManager;
	std::unique_ptr<ItemsSaveManager> itemsSaveManager;
	std::unique_ptr<UsersService> usersService;
};

Services::Services(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	// Создаём provider'ы
	d->inventoriesDataProvider = std::make_unique<InventoriesDataProviderJsonImpl>(resources);
	d->inventoryDataProvider = std::make_unique<InventoryDataProviderJsonImpl>(resources);
	d->itemsDataProvider = std::make_unique<ItemsDataProviderJsonImpl>(resources);
	d->equipmentDataProvider = std::make_unique<EquipmentDataProviderJsonImpl>(resources);
	d->inventoryDataWriter = std::make_unique<InventoryDataWriterJsonImpl>(resources);
	d->equipmentDataWriter = std::make_unique<EquipmentDataWriterJsonImpl>(resources);
	d->itemsDataWriter = std::make_unique<ItemsDataWriterJsonImpl>(resources);

	// Создаём репозитории (адаптеры к provider'ам)
	d->itemRepository = std::make_shared<ItemRepositoryJsonImpl>(d->itemsDataProvider.get());
	d->inventoryRepository = std::make_shared<InventoryRepositoryJsonImpl>(
		d->inventoryDataProvider.get(),
		d->inventoryDataWriter.get());

	d->equipmentRepository = std::make_shared<EquipmentRepositoryJsonImpl>(
		d->equipmentDataProvider.get(),
		d->equipmentDataWriter.get());

	d->timeService = std::make_unique<TimeService>();

	d->worldService = std::make_unique<WorldService>();

	// Передаём репозиторий в ItemsService (вместо provider)
	d->itemsService = std::make_unique<ItemsService>(d->itemRepository);

	// Создаём InventoryLoader перед InventoriesService (он нужен для загрузки)
	d->inventoryLoader = std::make_unique<InventoryLoader>(
		d->inventoryRepository,
		d->equipmentRepository,
		d->itemRepository,
		d->itemsService.get());

	d->inventoriesService = std::make_unique<InventoriesService>(
		d->itemsService.get(),
		d->inventoryLoader.get());

	d->inventoriesSaveManager = std::make_unique<InventoriesSaveManager>(
		d->inventoriesService.get(),
		d->inventoryDataWriter.get(),
		d->equipmentDataWriter.get());

	d->itemsSaveManager = std::make_unique<ItemsSaveManager>(
		d->itemsService.get(),
		d->itemsDataWriter.get());

	// Создаём сервис пользователей
	d->usersService = std::make_unique<UsersService>(
		std::make_unique<UsersDataProviderJsonImpl>(resources));

	// Подключаем сохранение к сигналу save()
	connect(this, &Services::save,
		d->inventoriesSaveManager.get(), &InventoriesSaveManager::saveAll);
	connect(this, &Services::save,
		d->itemsSaveManager.get(), &ItemsSaveManager::saveAll);
}

Services::~Services() = default;

void Services::run() {
	// Загружаем все сущности предметов (eager loading для сущностей)
	d->itemsService->loadEntities();
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

ItemsService* Services::itemsService() const {
	return d->itemsService.get();
}

InventoriesService* Services::inventoriesService() const {
	return d->inventoriesService.get();
}

InventoryLoader* Services::inventoryLoader() const {
	return d->inventoryLoader.get();
}

IInventoryRepository* Services::inventoryRepository() const {
	return d->inventoryRepository.get();
}

UsersService* Services::usersService() const {
	return d->usersService.get();
}
