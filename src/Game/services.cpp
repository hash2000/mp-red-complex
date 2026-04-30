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
#include "DataLayer/characters/character_data_provider_json_impl.h"
#include "DataLayer/images/images_data_provider_json_impl.h"
#include "DataLayer/images/i_images_data_provider.h"
#include "DataLayer/images/tile_groups_data_provider_json_impl.h"
#include "DataLayer/images/i_tile_groups_data_provider.h"
#include "DataLayer/maps/i_map_data_provider.h"
#include "DataLayer/maps/map_data_provider_json_impl.h"
#include "DataLayer/users/i_users_data_provider.h"
#include "DataLayer/characters/i_character_data_provider.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/inventory_loader.h"
#include "ApplicationLayer/inventories_save_manager.h"
#include "ApplicationLayer/items_save_manager.h"
#include "ApplicationLayer/users/users_service.h"
#include "ApplicationLayer/textures/images_service.h"
#include "ApplicationLayer/maps/map_service.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "ApplicationLayer/textures/tiles_selector_service.h"

class Services::Private {
public:
	Private(Services* parent) : q(parent) {
	}

	Services* q;
	Resources* resources;
	std::unique_ptr<TimeService> timeService;
	std::unique_ptr<WorldService> worldService;

private:
	// Data Providers (остаются для обратной совместимости и writer'ов)
	std::unique_ptr<IInventoryDataProvider> inventoryDataProvider;
	std::unique_ptr<IInventoryDataWriter> inventoryDataWriter;
	std::unique_ptr<IItemsDataProvider> itemsDataProvider;
	std::unique_ptr<IItemsDataWriter> itemsDataWriter;
	std::unique_ptr<IEquipmentDataProvider> equipmentDataProvider;
	std::unique_ptr<IEquipmentDataWriter> equipmentDataWriter;
	std::unique_ptr<IInventoriesDataProvider> inventoriesDataProvider;
	std::unique_ptr<IUsersDataProvider> usersDataProvider;
	std::unique_ptr<ICharacterDataProvider> characterDataProvider;
	std::unique_ptr<IImagesDataProvider> imagesDataProvider;
	std::unique_ptr<ITileGroupsDataProvider> tileGroupsDataProvider;
	std::unique_ptr<IMapDataProvider> mapDataProvider;

	// Repositories (новый слой абстракции)
	std::shared_ptr<IItemRepository> itemRepository;
	std::shared_ptr<IInventoryRepository> inventoryRepository;
	std::shared_ptr<IEquipmentRepository> equipmentRepository;

	// Save managers
	std::unique_ptr<InventoriesSaveManager> inventoriesSaveManager;
	std::unique_ptr<ItemsSaveManager> itemsSaveManager;

public:
	// Services
	std::unique_ptr<ItemsService> itemsService;
	std::unique_ptr<InventoriesService> inventoriesService;
	std::unique_ptr<InventoryLoader> inventoryLoader;

	std::unique_ptr<UsersService> usersService;
	std::unique_ptr<ImagesService> imagesService;
	std::unique_ptr<TilesSelectorService> tilesSelectorService;
	std::unique_ptr<TexturesService> texturesService;

	std::unique_ptr<MapService> mapService;

public:
	IInventoriesDataProvider* getInventoriesDataProvider() {
		if (!inventoriesDataProvider) {
			inventoriesDataProvider = std::make_unique<InventoriesDataProviderJsonImpl>(resources);
		}
		return inventoriesDataProvider.get();
	}

	IInventoryDataProvider* getInventoryDataProvider() {
		if (!inventoryDataProvider) {
			inventoryDataProvider = std::make_unique<InventoryDataProviderJsonImpl>(resources);
		}

		return inventoryDataProvider.get();
	}

	IItemsDataProvider* getItemsDataProvider() {
		if (!itemsDataProvider) {
			itemsDataProvider = std::make_unique<ItemsDataProviderJsonImpl>(resources);
		}
		return itemsDataProvider.get();
	}

	IEquipmentDataProvider* getEquipmentDataProvider() {
		if (!equipmentDataProvider) {
			equipmentDataProvider = std::make_unique<EquipmentDataProviderJsonImpl>(resources);
		}
		return equipmentDataProvider.get();
	}

	IInventoryDataWriter* getInventoryDataWriter() {
		if (!inventoryDataWriter) {
			inventoryDataWriter = std::make_unique<InventoryDataWriterJsonImpl>(resources);
		}
		return inventoryDataWriter.get();
	}

	IEquipmentDataWriter* getEquipmentDataWriter() {
		if (!equipmentDataWriter) {
			equipmentDataWriter = std::make_unique<EquipmentDataWriterJsonImpl>(resources);
		}
		return equipmentDataWriter.get();
	}

	IItemsDataWriter* getItemsDataWriter() {
		if (!itemsDataWriter) {
			itemsDataWriter = std::make_unique<ItemsDataWriterJsonImpl>(resources);
		}
		return itemsDataWriter.get();
	}

	IItemRepository* getItemRepository() {
		if (!itemRepository) {
			itemRepository = std::make_shared<ItemRepositoryJsonImpl>(getItemsDataProvider());
		}
		return itemRepository.get();
	}

	IInventoryRepository* getInventoryRepository() {
		if (!inventoryRepository) {
			inventoryRepository = std::make_shared<InventoryRepositoryJsonImpl>(
				getInventoryDataProvider(),
				getInventoryDataWriter());
		}
		return inventoryRepository.get();
	}

	IEquipmentRepository* getEquipmentRepository() {
		if (!equipmentRepository) {
			equipmentRepository = std::make_shared<EquipmentRepositoryJsonImpl>(
				getEquipmentDataProvider(),
				getEquipmentDataWriter());
		}
		return equipmentRepository.get();
	}

	IImagesDataProvider* getImagesDataProvider() {
		if (!imagesDataProvider) {
			imagesDataProvider = std::make_unique<ImagesDataProviderJsonImpl>(resources);
		}
		return imagesDataProvider.get();
	}

	ITileGroupsDataProvider* getTileGroupsDataProvider() {
		if (!tileGroupsDataProvider) {
			tileGroupsDataProvider = std::make_unique<TileGroupsDataProviderJsonImpl>(resources);
		}
		return tileGroupsDataProvider.get();
	}

	IMapDataProvider* getMapDataProvider() {
		if (!mapDataProvider) {
			mapDataProvider = std::make_unique<MapDataProviderJsonImpl>(resources);
		}
		return mapDataProvider.get();
	}

	IUsersDataProvider* getUsersDataProvider() {
		if (!usersDataProvider) {
			usersDataProvider = std::make_unique<UsersDataProviderJsonImpl>(resources);
		}
		return usersDataProvider.get();
	}

	ICharacterDataProvider* getCharacterDataProvider() {
		if (!characterDataProvider) {
			characterDataProvider = std::make_unique<CharacterDataProviderJsonImpl>(resources);
		}
		return characterDataProvider.get();
	}

	InventoriesSaveManager* getInventoriesSaveManager() {
		if (!inventoriesSaveManager) {
			inventoriesSaveManager = std::make_unique<InventoriesSaveManager>(
				q->inventoriesService(),
				getInventoryDataWriter(),
				getEquipmentDataWriter());
		}
		return inventoriesSaveManager.get();
	}

	ItemsSaveManager* getItemsSaveManager() {
		if (!itemsSaveManager) {
			itemsSaveManager = std::make_unique<ItemsSaveManager>(
				q->itemsService(),
				getItemsDataWriter());
		}
		return itemsSaveManager.get();
	}
};

Services::Services(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;

	// Подключаем сохранение к сигналу save()
	connect(this, &Services::save,
		d->getInventoriesSaveManager(), &InventoriesSaveManager::saveAll);
	connect(this, &Services::save,
		d->getItemsSaveManager(), &ItemsSaveManager::saveAll);
}

Services::~Services() = default;

void Services::run() {
	d->itemsService->loadEntities();
	timeService()->start();
}

void Services::postLoadEvent() {
	emit load();
}

void Services::postSaveEvent() {
	emit save();
}

TimeService* Services::timeService() const {
	if (!d->timeService) {
		d->timeService = std::make_unique<TimeService>();
	}
	return d->timeService.get();
}

WorldService* Services::worldService() const {
	if (!d->worldService) {
		d->worldService = std::make_unique<WorldService>();
	}
	return d->worldService.get();
}

ItemsService* Services::itemsService() const {
	if (!d->itemsService) {
		d->itemsService = std::make_unique<ItemsService>(
			d->getItemRepository(),
			imagesService());
	}
	return d->itemsService.get();
}

InventoriesService* Services::inventoriesService() const {
	if (!d->inventoriesService) {
		d->inventoriesService = std::make_unique<InventoriesService>(
			itemsService(),
			inventoryLoader());
	}
	return d->inventoriesService.get();
}

InventoryLoader* Services::inventoryLoader() const {
	if (!d->inventoryLoader) {
		d->inventoryLoader = std::make_unique<InventoryLoader>(
			d->getInventoryRepository(),
			d->getEquipmentRepository(),
			d->getItemRepository(),
			itemsService());
	}
	return d->inventoryLoader.get();
}

UsersService* Services::usersService() const {
	if (!d->usersService) {
		d->usersService = std::make_unique<UsersService>(
			d->getUsersDataProvider(),
			d->getCharacterDataProvider(),
			imagesService());
	}
	return d->usersService.get();
}

ImagesService* Services::imagesService() const {
	if (!d->imagesService) {
		d->imagesService = std::make_unique<ImagesService>(
			d->getImagesDataProvider());
	}
	return d->imagesService.get();
}

TilesSelectorService* Services::tilesSelectorService() const {
	if (!d->tilesSelectorService) {
		d->tilesSelectorService = std::make_unique<TilesSelectorService>(
			d->getTileGroupsDataProvider());
	}
	return d->tilesSelectorService.get();
}

MapService* Services::mapService() const {
	if (!d->mapService) {
		d->mapService = std::make_unique<MapService>(
			tilesSelectorService(),
			imagesService(),
			d->getMapDataProvider());
	}
	return d->mapService.get();
}

TexturesService* Services::texturesService() const {
	if (!d->texturesService) {
		d->texturesService = std::make_unique<TexturesService>(
			imagesService());
	}
	return d->texturesService.get();
}
