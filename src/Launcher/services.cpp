#include "Launcher/services.h"
#include "Libs/Engine/services/i_services_factory.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Resources/resources.h"

// FetchApiModule
#include "Content/FetchApiModule/services/fetch_service.h"

// ShadersModule
#include "Content/ShadersModule/data_providers/shaders_data_provider_impl.h"
#include "Content/ShadersModule/services/shaders_service.h"

// MaterialsModule
#include "Content/MaterialsModule/data_providers/material_data_provider_json_impl.h"
#include "Content/MaterialsModule/services/materials_service.h"

// TexturesModeule
#include "Content/TexturesModule/data_providers/images_data_provider_json_impl.h"
#include "Content/TexturesModule/data_providers/i_images_data_provider.h"
#include "Content/TexturesModule/data_providers/tile_groups_data_provider_json_impl.h"
#include "Content/TexturesModule/data_providers/i_tile_groups_data_provider.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Content/TexturesModule/services/textures_service.h"
#include "Content/TexturesModule/services/tiles_selector_service.h"

// Items service
#include "Content/InventoriesModule/data_providers/items_data_provider_db.h"
#include "Content/InventoriesModule/data_providers/entities_data_provider_db.h"
#include "Content/InventoriesModule/services/items_service.h"

// DatabasesService
#include "Content/DatabaseModule/data_providers/databases_settings_data_provider_json_impl.h"
#include "Content/DatabaseModule/services/databases_service.h"

// CharactersModule
#include "Content/CharactersModule/data_providers/character_data_provider_db.h"
#include "Content/CharactersModule/services/characters_service.h"

// UsersModule
#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/data_providers/users/users_data_provider_db.h"

// HighlightingPluginManager
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"

#include <mutex>

namespace {
class ServicesFactory : public ServicesRegistry, public IServicesFactory {
public:
	~ServicesFactory() = default;
	ServicesFactory(Resources* res)
		// Data Providers
		: resources(res)
		, databaseSettingsDataProvider([this] { return std::make_unique<DatabaseSettingsDataProviderJsonImpl>(resources); })
		, itemsDataProvider([this] { return std::make_unique<ItemsDataProviderDb>(databasesService.get()); })
		, entitiesDataProvider([this] { return std::make_unique<EntitiesDataProviderDb>(databasesService.get()); })
		, imagesDataProvider([this] {	return std::make_unique<ImagesDataProviderJsonImpl>(resources);	})
		, tileGroupsDataProvider([this] {	return std::make_unique<TileGroupsDataProviderJsonImpl>(resources);	})
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
	{}

private:
	Resources* resources;

private:
	// Data Providers (остаются для обратной совместимости и writer'ов)
	LazyPtr<DatabaseSettingsDataProviderJsonImpl> databaseSettingsDataProvider;
	LazyPtr<IItemsDataProvider> itemsDataProvider;
	LazyPtr<IEntitiesDataProvider> entitiesDataProvider;
	LazyPtr<IUsersDataProvider> usersDataProvider;
	LazyPtr<ICharacterDataProvider> characterDataProvider;
	LazyPtr<IImagesDataProvider> imagesDataProvider;
	LazyPtr<ITileGroupsDataProvider> tileGroupsDataProvider;
	LazyPtr<IShadersDataProvider> shadersDataProvider;
	LazyPtr<IMaterialsDataProvider> materialsDataProvider;

	// Services
	LazyPtr<DatabasesService> databasesService;

public:
	void registerServices() {

		registerFactory<ItemsService>([this] {
			return std::make_unique<ItemsService>(
				this->get<ImagesService>(),
				entitiesDataProvider.get(),
				itemsDataProvider.get());
		});

		registerFactory<UsersService>([this] {
			return std::make_unique<UsersService>(
				resources,
				usersDataProvider.get(),
				this->get<ImagesService>());
		});

		registerFactory<ImagesService>([this] {
			return std::make_unique<ImagesService>(
				imagesDataProvider.get());
		});

		registerFactory<TilesSelectorService>([this] {
			return std::make_unique<TilesSelectorService>(
				tileGroupsDataProvider.get());
		});

		registerFactory<TexturesService>([this] {
			return std::make_unique<TexturesService>(
				this->get<ImagesService>());
		});

		registerFactory<MaterialsService>([this] {
			return std::make_unique<MaterialsService>(
				materialsDataProvider.get());
		});

		registerFactory<CharactersService>([this] {
			return std::make_unique<CharactersService>(
				characterDataProvider.get(),
				this->get<UsersService>(),
				this->get<ImagesService>());
		});

		registerFactory<CharactersService>([this] {
			return std::make_unique<CharactersService>(
				characterDataProvider.get(),
				this->get<UsersService>(),
				this->get<ImagesService>());
		});

		registerFactory<FetchApiService>([this]() {
			return std::make_unique<FetchApiService>();
		});

		registerFactory<HighlightingPluginManager>([this] {
			return std::make_unique<HighlightingPluginManager>();
		});

		registerFactory<FormatterPluginManager>([this] {
			return std::make_unique<FormatterPluginManager>(
				resources);
		});
	}
};
}

class Services::Private {
public:
	Private(Services* parent) : q(parent) {}
	Services* q;
	Resources* resources;
};

Services::Services(Resources* resources)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

Services::~Services() = default;

std::unique_ptr<ServicesRegistry> Services::create() {
	auto factory = std::make_unique<ServicesFactory>(d->resources);
	factory->registerServices();
	return factory;
}

