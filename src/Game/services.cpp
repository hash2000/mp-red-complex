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
#include "DataLayer/shaders/shaders_data_provider_impl.h"

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

#include <mutex>

class Services::Private {
public:
	template<typename T>
	class LazyPtr {
		std::unique_ptr<T> _ptr;
		std::once_flag _flag;
		std::function<std::unique_ptr<T>()> _factory;

	public:
		explicit LazyPtr(std::function<std::unique_ptr<T>()> factory)
			: _factory(std::move(factory)) {
		}

		T* get() {
			std::call_once(_flag, [this] {
				_ptr = _factory();
				});
			return _ptr.get();
		}

		T* operator->() {
			return get();
		}

		T& operator*() {
			return *get();
		}

		bool isInitialized() const noexcept {
			return static_cast<bool>(_ptr);
		}
	};
public:
	Private(Services* parent)
		: q(parent)

		// Data Providers
		, inventoryDataProvider([this] { return std::make_unique<InventoryDataProviderJsonImpl>(resources); })
		, inventoryDataWriter([this] { return std::make_unique<InventoryDataWriterJsonImpl>(resources); })
		, shadersDataProvider([this] { return std::make_unique<ShadersDataProviderLocalImpl>(resources); })
		, inventoriesDataProvider([this] { return std::make_unique<InventoriesDataProviderJsonImpl>(resources); })
		, itemsDataProvider([this] { return std::make_unique<ItemsDataProviderJsonImpl>(resources); })
		, equipmentDataProvider([this] { return std::make_unique<EquipmentDataProviderJsonImpl>(resources); })
		, equipmentDataWriter([this] { return std::make_unique<EquipmentDataWriterJsonImpl>(resources); })
		, itemsDataWriter([this] { return std::make_unique<ItemsDataWriterJsonImpl>(resources); })
		, imagesDataProvider([this] {	return std::make_unique<ImagesDataProviderJsonImpl>(resources);	})
		, tileGroupsDataProvider([this] {	return std::make_unique<TileGroupsDataProviderJsonImpl>(resources);	})
		, mapDataProvider([this] {	return std::make_unique<MapDataProviderJsonImpl>(resources);	})
		, usersDataProvider([this] {	return std::make_unique<UsersDataProviderJsonImpl>(resources);	})
		, characterDataProvider([this] {	return std::make_unique<CharacterDataProviderJsonImpl>(resources);	})

		// Repositories
		, itemRepository([this] {
				return std::make_unique<ItemRepositoryJsonImpl>(
					itemsDataProvider.get());
			})
		, inventoryRepository([this] {
				return std::make_unique<InventoryRepositoryJsonImpl>(
					inventoryDataProvider.get(),
					inventoryDataWriter.get());
			})
		, equipmentRepository([this] {
				return std::make_unique<EquipmentRepositoryJsonImpl>(
					equipmentDataProvider.get(),
					equipmentDataWriter.get());
			})

		// Save managers
		, inventoriesSaveManager([this] {
				return std::make_unique<InventoriesSaveManager>(
					inventoriesService.get(),
					inventoryDataWriter.get(),
					equipmentDataWriter.get());
			})
		, itemsSaveManager([this] {
				return std::make_unique<ItemsSaveManager>(
					itemsService.get(),
					itemsDataWriter.get());
			})

		// Services
		, timeService([] { return std::make_unique<TimeService>(); })
		, worldService([] { return std::make_unique<WorldService>(); })
		, itemsService([this] {
				return std::make_unique<ItemsService>(
					itemRepository.get(),
					imagesService.get());
			})
		, inventoriesService([this] {
				return std::make_unique<InventoriesService>(
					itemsService.get(),
					inventoryLoader.get());
			})
		, inventoryLoader([this] {
				return std::make_unique<InventoryLoader>(
					inventoryRepository.get(),
					equipmentRepository.get(),
					itemRepository.get(),
					itemsService.get());
			})
		, usersService([this] {
				return std::make_unique<UsersService>(
					usersDataProvider.get(),
					characterDataProvider.get(),
					imagesService.get());
			})
		, imagesService([this] {
				return std::make_unique<ImagesService>(
					imagesDataProvider.get());
			})
		, tilesSelectorService([this] {
				return std::make_unique<TilesSelectorService>(
					tileGroupsDataProvider.get());
			})
		, mapService([this] {
				return std::make_unique<MapService>(
					tilesSelectorService.get(),
					imagesService.get(),
					mapDataProvider.get());
			})
		, texturesService([this] {
				return std::make_unique<TexturesService>(
					imagesService.get());
			})
	{
	}

	Services* q;
	Resources* resources;

public:
	LazyPtr<TimeService> timeService;
	LazyPtr<WorldService> worldService;

	// Data Providers (остаются для обратной совместимости и writer'ов)
	LazyPtr<IInventoryDataProvider> inventoryDataProvider;
	LazyPtr<IInventoryDataWriter> inventoryDataWriter;
	LazyPtr<IItemsDataProvider> itemsDataProvider;
	LazyPtr<IItemsDataWriter> itemsDataWriter;
	LazyPtr<IEquipmentDataProvider> equipmentDataProvider;
	LazyPtr<IEquipmentDataWriter> equipmentDataWriter;
	LazyPtr<IInventoriesDataProvider> inventoriesDataProvider;
	LazyPtr<IUsersDataProvider> usersDataProvider;
	LazyPtr<ICharacterDataProvider> characterDataProvider;
	LazyPtr<IImagesDataProvider> imagesDataProvider;
	LazyPtr<ITileGroupsDataProvider> tileGroupsDataProvider;
	LazyPtr<IMapDataProvider> mapDataProvider;
	LazyPtr<IShadersDataProvider> shadersDataProvider;

	// Repositories (новый слой абстракции)
	LazyPtr<IItemRepository> itemRepository;
	LazyPtr<IInventoryRepository> inventoryRepository;
	LazyPtr<IEquipmentRepository> equipmentRepository;

	// Save managers
	LazyPtr<InventoriesSaveManager> inventoriesSaveManager;
	LazyPtr<ItemsSaveManager> itemsSaveManager;

	// Services
	LazyPtr<ItemsService> itemsService;
	LazyPtr<InventoriesService> inventoriesService;
	LazyPtr<InventoryLoader> inventoryLoader;

	LazyPtr<UsersService> usersService;
	LazyPtr<ImagesService> imagesService;
	LazyPtr<TilesSelectorService> tilesSelectorService;
	LazyPtr<TexturesService> texturesService;

	LazyPtr<MapService> mapService;

};

Services::Services(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;

	// Подключаем сохранение к сигналу save()
	connect(this, &Services::save,
		d->inventoriesSaveManager.get(), &InventoriesSaveManager::saveAll);
	connect(this, &Services::save,
		d->itemsSaveManager.get(), &ItemsSaveManager::saveAll);
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

UsersService* Services::usersService() const {
	return d->usersService.get();
}

ImagesService* Services::imagesService() const {
	return d->imagesService.get();
}

TilesSelectorService* Services::tilesSelectorService() const {
	return d->tilesSelectorService.get();
}

MapService* Services::mapService() const {
	return d->mapService.get();
}

TexturesService* Services::texturesService() const {
	return d->texturesService.get();
}
