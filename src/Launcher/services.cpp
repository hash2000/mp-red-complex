#include "Launcher/services.h"
#include "Launcher/services/time_service/time_service.h"
#include "Launcher/services/world_service/world_service.h"
#include "Libs/Resources/resources.h"

#include "DataLayer/images/images_data_provider_json_impl.h"
#include "DataLayer/images/i_images_data_provider.h"
#include "DataLayer/images/tile_groups_data_provider_json_impl.h"
#include "DataLayer/images/i_tile_groups_data_provider.h"
#include "DataLayer/maps/i_map_data_provider.h"
#include "DataLayer/maps/map_data_provider_json_impl.h"
#include "DataLayer/shaders/shaders_data_provider_impl.h"
#include "DataLayer/materials/material_data_provider_json_impl.h"



#include "ApplicationLayer/textures/images_service.h"
#include "ApplicationLayer/maps/map_service.h"
#include "ApplicationLayer/textures/textures_service.h"
#include "ApplicationLayer/textures/tiles_selector_service.h"
#include "ApplicationLayer/shaders/shaders_service.h"
#include "ApplicationLayer/materials/materials_service.h"

// Items service
#include "Content/InventroiesModule/data_providers/items_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/entities_data_provider_db.h"
#include "Content/InventroiesModule/services/items_service.h"

// DatabasesService
#include "Content/DatabaseModule/data_providers/databases_settings_data_provider_json_impl.h"
#include "Content/DatabaseModule/services/databases_service.h"

// UsersModule
#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/data_providers/characters/character_data_provider_db.h"
#include "Content/UsersModule/data_providers/users/users_data_provider_db.h"

// HighlightingPluginManager
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"

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

		, highlightingPluginManager([this] {
				auto manager = std::make_unique<HighlightingPluginManager>();
				manager->loadPlugins(resources->Variables.get("Plugins.Path", "").toString());
				return manager;
		})

		// Data Providers
		, databaseSettingsDataProvider([this] { return std::make_unique<DatabaseSettingsDataProviderJsonImpl>(resources); })
		, itemsDataProvider([this] { return std::make_unique<ItemsDataProviderDb>(databasesService.get()); })
		, entitiesDataProvider([this] { return std::make_unique<EntitiesDataProviderDb>(databasesService.get()); })
		, imagesDataProvider([this] {	return std::make_unique<ImagesDataProviderJsonImpl>(resources);	})
		, tileGroupsDataProvider([this] {	return std::make_unique<TileGroupsDataProviderJsonImpl>(resources);	})
		, mapDataProvider([this] {	return std::make_unique<MapDataProviderJsonImpl>(resources);	})
		, usersDataProvider([this] {	return std::make_unique<UsersDataProviderDb>(databasesService.get());	})
		, characterDataProvider([this] {	return std::make_unique<CharacterDataProviderDb>(databasesService.get());	})
		, shadersDataProvider([this] { return std::make_unique<ShadersDataProviderLocalImpl>(resources); })
		, materialsDataProvider([this] { return std::make_unique<MaterialsDataProviderJsonImpl>(resources); })

		// Services
		, databasesService([this] {
				return std::make_unique<DatabasesService>(
					resources,
					databaseSettingsDataProvider.get());
			})
		, timeService([] { return std::make_unique<TimeService>(); })
		, worldService([] { return std::make_unique<WorldService>(); })
		, itemsService([this] {
				return std::make_unique<ItemsService>(
					imagesService.get(),
					entitiesDataProvider.get(),
					itemsDataProvider.get());
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
		, materialsService([this] {
				return std::make_unique<MaterialsService>(
					materialsDataProvider.get());
			})
	{
	}

	Services* q;
	Resources* resources;

public:
	LazyPtr<TimeService> timeService;
	LazyPtr<WorldService> worldService;

	// Data Providers (остаются для обратной совместимости и writer'ов)
	LazyPtr<DatabaseSettingsDataProviderJsonImpl> databaseSettingsDataProvider;
	LazyPtr<IItemsDataProvider> itemsDataProvider;
	LazyPtr<IEntitiesDataProvider> entitiesDataProvider;
	LazyPtr<IUsersDataProvider> usersDataProvider;
	LazyPtr<ICharacterDataProvider> characterDataProvider;
	LazyPtr<IImagesDataProvider> imagesDataProvider;
	LazyPtr<ITileGroupsDataProvider> tileGroupsDataProvider;
	LazyPtr<IMapDataProvider> mapDataProvider;
	LazyPtr<IShadersDataProvider> shadersDataProvider;
	LazyPtr<IMaterialsDataProvider> materialsDataProvider;

	// Services
	LazyPtr<DatabasesService> databasesService;
	LazyPtr<ItemsService> itemsService;
	LazyPtr<UsersService> usersService;
	LazyPtr<ImagesService> imagesService;
	LazyPtr<TilesSelectorService> tilesSelectorService;
	LazyPtr<TexturesService> texturesService;
	LazyPtr<MapService> mapService;
	LazyPtr<MaterialsService> materialsService;

	LazyPtr<HighlightingPluginManager> highlightingPluginManager;
};

Services::Services(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

Services::~Services() = default;

void Services::run() {
	timeService()->start();
}

void Services::postLoadEvent() {
	emit load();
}

void Services::postSaveEvent() {
	emit save();
}

DatabasesService* Services::databasesService() const {
	return d->databasesService.get();
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

MaterialsService* Services::materialsService() const {
	return d->materialsService.get();
}

std::unique_ptr<ShadersService> Services::shadersService() const {
	return std::move(std::make_unique<ShadersService>(
		d->shadersDataProvider.get()));
}

HighlightingPluginManager* Services::highlightingPluginManager() const {
	return d->highlightingPluginManager.get();
}
