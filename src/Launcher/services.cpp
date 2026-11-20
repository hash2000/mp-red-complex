#include "Launcher/services.h"
#include "Libs/Engine/services/i_services_factory.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Resources/resources.h"

// FetchApiModule
#include "Content/FetchApiModule/data_providers/fetch_queries_data_provider_db.h"
#include "Content/FetchApiModule/services/fetch_queries_service.h"
#include "Content/FetchApiModule/services/fetch_service.h"

// SystemRouter
#include "Content/SystemRouter/data_providers/system_router_commands_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_data_provider_db.h"
#include "Content/SystemRouter/services/system_router_command_service.h"
#include "Content/SystemRouter/services/system_router_service.h"

//// ShadersModule
//#include "Content/ShadersModule/data_providers/shaders_data_provider_impl.h"
//#include "Content/ShadersModule/services/shaders_service.h"
//
//// MaterialsModule
//#include "Content/MaterialsModule/data_providers/material_data_provider_json_impl.h"
//#include "Content/MaterialsModule/services/materials_service.h"
//
//// TexturesModeule
//#include "Content/TexturesModule/data_providers/images_data_provider_json_impl.h"
//#include "Content/TexturesModule/data_providers/i_images_data_provider.h"
//#include "Content/TexturesModule/data_providers/tile_groups_data_provider_json_impl.h"
//#include "Content/TexturesModule/data_providers/i_tile_groups_data_provider.h"
//#include "Content/TexturesModule/services/images_service.h"
//#include "Content/TexturesModule/services/textures_service.h"
//#include "Content/TexturesModule/services/tiles_selector_service.h"
//
//// Items service
//#include "Content/InventoriesModule/data_providers/items_data_provider_db.h"
//#include "Content/InventoriesModule/data_providers/entities_data_provider_db.h"
//#include "Content/InventoriesModule/services/items_service.h"
//
// DatabasesService
#include "Content/DatabaseModule/services/databases_service.h"

//// CharactersModule
//#include "Content/CharactersModule/data_providers/character_data_provider_db.h"
//#include "Content/CharactersModule/services/characters_service.h"

// UsersModule
#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/services/users_registry_service.h"
#include "Content/UsersModule/data_providers/users_registry/users_registry_data_provider_db.h"

// HighlightingPluginManager
#include "Content/CodeEditorWidget/highlights/highlighter_plugin_manager.h"
#include "Content/CodeEditorWidget/formatters/formatter_plugin_manager.h"

#include <mutex>
#include <utility>

namespace {
class ServicesFactory : public ServicesRegistry, public IServicesFactory {
public:
	~ServicesFactory() = default;
	ServicesFactory(Resources* res, std::shared_ptr<CommandContext> context)
		// Data Providers
		: resources(res)
		, globalContext(std::move(context))
		//, itemsDataProvider([this] { return std::make_unique<ItemsDataProviderDb>(databasesService.get()); })
		//, entitiesDataProvider([this] { return std::make_unique<EntitiesDataProviderDb>(databasesService.get()); })
		//, imagesDataProvider([this] {	return std::make_unique<ImagesDataProviderJsonImpl>(resources);	})
		//, tileGroupsDataProvider([this] {	return std::make_unique<TileGroupsDataProviderJsonImpl>(resources);	})
		, usersRegistryDataProvider([this] {	return std::make_unique<UsersRegistryDataProviderDb>(databasesService.get()); })
		, fetchQueriesDataProvider([this] {	return std::make_unique<FetchQueriesDataProviderDb>(databasesService.get()); })
		, systemRouterDataProvider([this] {	return std::make_unique<SystemRouterDataProviderDb>(databasesService.get()); })
		, systemRouterCommandsProvider([this] {	return std::make_unique<SystemRouterCommandsProviderDb>(databasesService.get()); })
		//, characterDataProvider([this] {	return std::make_unique<CharacterDataProviderDb>(databasesService.get());	})
		//, shadersDataProvider([this] { return std::make_unique<ShadersDataProviderLocalImpl>(resources); })
		//, materialsDataProvider([this] { return std::make_unique<MaterialsDataProviderJsonImpl>(resources); })

		// Services
		, databasesService([this] {
			return std::make_unique<DatabasesService>(
				resources,
				globalContext);
		})
	{}

private:
	Resources* resources;
	std::shared_ptr<CommandContext> globalContext;

private:
	// Data Providers (остаются для обратной совместимости и writer'ов)
	//LazyPtr<IItemsDataProvider> itemsDataProvider;
	//LazyPtr<IEntitiesDataProvider> entitiesDataProvider;
	LazyPtr<IUsersRegistryDataProvider> usersRegistryDataProvider;
	LazyPtr<FetchQueriesDataProviderDb> fetchQueriesDataProvider;
	LazyPtr<SystemRouterDataProviderDb> systemRouterDataProvider;
	LazyPtr<SystemRouterCommandsProviderDb> systemRouterCommandsProvider;
	//LazyPtr<ICharacterDataProvider> characterDataProvider;
	//LazyPtr<IImagesDataProvider> imagesDataProvider;
	//LazyPtr<ITileGroupsDataProvider> tileGroupsDataProvider;
	//LazyPtr<IShadersDataProvider> shadersDataProvider;
	//LazyPtr<IMaterialsDataProvider> materialsDataProvider;

	// Services
	LazyPtr<DatabasesService> databasesService;

public:
	void registerServices() {

		//registerFactory<ItemsService>([this] {
		//	return std::make_unique<ItemsService>(
		//		this->get<ImagesService>(),
		//		entitiesDataProvider.get(),
		//		itemsDataProvider.get());
		//});

		registerFactory<UsersService>([this] {
			return std::make_unique<UsersService>(
				resources,
				databasesService.get(),
				usersRegistryDataProvider.get());
		});

		registerFactory<UsersRegistryService>([this] {
			return std::make_unique<UsersRegistryService>(
				resources,
				usersRegistryDataProvider.get());
		});

		//registerFactory<ImagesService>([this] {
		//	return std::make_unique<ImagesService>(
		//		imagesDataProvider.get());
		//});

		//registerFactory<TilesSelectorService>([this] {
		//	return std::make_unique<TilesSelectorService>(
		//		tileGroupsDataProvider.get());
		//});

		//registerFactory<TexturesService>([this] {
		//	return std::make_unique<TexturesService>(
		//		this->get<ImagesService>());
		//});

		//registerFactory<MaterialsService>([this] {
		//	return std::make_unique<MaterialsService>(
		//		materialsDataProvider.get());
		//});

		//registerFactory<CharactersService>([this] {
		//	return std::make_unique<CharactersService>(
		//		characterDataProvider.get(),
		//		this->get<UsersService>(),
		//		this->get<ImagesService>());
		//});

		//registerFactory<CharactersService>([this] {
		//	return std::make_unique<CharactersService>(
		//		characterDataProvider.get(),
		//		this->get<UsersService>(),
		//		this->get<ImagesService>());
		//});

		registerFactory<FetchApiService>([this]() {
			return std::make_unique<FetchApiService>();
		});

		registerFactory<FetchQueriesService>([this]() {
			return std::make_unique<FetchQueriesService>(
				fetchQueriesDataProvider.get());
		});

		registerFactory<SystemRouterService>([this]() {
			return std::make_unique<SystemRouterService>(
				systemRouterDataProvider.get());
		});

		registerFactory<SystemRouterCommandService>([this]() {
			return std::make_unique<SystemRouterCommandService>(
				systemRouterCommandsProvider.get());
		});

		registerFactory<HighlightingPluginManager>([this] {
			return std::make_unique<HighlightingPluginManager>(resources);
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
	std::shared_ptr<CommandContext> globalContext;
};

Services::Services(Resources* resources, std::shared_ptr<CommandContext> globalContext)
	: d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->globalContext = std::move(globalContext);
}

Services::~Services() = default;

std::unique_ptr<ServicesRegistry> Services::create() {
	auto factory = std::make_unique<ServicesFactory>(d->resources, d->globalContext);
	factory->registerServices();
	return factory;
}

