#include "Launcher/services.h"
#include "Libs/Engine/services/i_services_factory.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Resources/resources.h"

// FetchApiModule
#include "Content/FetchApiModule/services/fetch_service.h"

// SystemRouter
#include "Content/DatabaseModule/data_providers/data_blobs_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_commands_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_data_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_db.h"
#include "Content/SystemRouter/services/system_router_blob_service.h"
#include "Content/SystemRouter/services/system_router_command_service.h"
#include "Content/SystemRouter/services/system_router_service.h"

// DatabasesService
#include "Content/DatabaseModule/services/databases_service.h"

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
		, usersRegistryDataProvider([this] {	return std::make_unique<UsersRegistryDataProviderDb>(databasesService.get()); })
		, systemRouterDataProvider([this] {	return std::make_unique<SystemRouterDataProviderDb>(databasesService.get()); })
		, systemRouterCommandsProvider([this] {	return std::make_unique<SystemRouterCommandsProviderDb>(databasesService.get()); })
		, systemRouterBlobsProvider([this] {
			return std::make_unique<DataBlobsProviderDb>(databasesService.get(),
				QString::fromLatin1(SystemRouterDb::kDatabase),
				QString::fromLatin1(SystemRouterDb::kDataBlobsTable));
		})

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
	LazyPtr<IUsersRegistryDataProvider> usersRegistryDataProvider;
	LazyPtr<SystemRouterDataProviderDb> systemRouterDataProvider;
	LazyPtr<SystemRouterCommandsProviderDb> systemRouterCommandsProvider;
	LazyPtr<DataBlobsProviderDb> systemRouterBlobsProvider;

	// Services
	LazyPtr<DatabasesService> databasesService;

public:
	void registerServices() {

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

		registerFactory<FetchApiService>([this]() {
			return std::make_unique<FetchApiService>();
		});

		registerFactory<SystemRouterService>([this]() {
			return std::make_unique<SystemRouterService>(
				systemRouterDataProvider.get());
		});

		registerFactory<SystemRouterCommandService>([this]() {
			return std::make_unique<SystemRouterCommandService>(
				systemRouterCommandsProvider.get());
		});

		registerFactory<SystemRouterBlobService>([this]() {
			return std::make_unique<SystemRouterBlobService>(
				systemRouterBlobsProvider.get());
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

