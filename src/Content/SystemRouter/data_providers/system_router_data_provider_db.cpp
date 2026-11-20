#include "Content/SystemRouter/data_providers/system_router_data_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_db.h"

SystemRouterDataProviderDb::SystemRouterDataProviderDb(DatabasesService* databasesService)
	: LazyNodesDataProviderDb(databasesService, SystemRouterDb::kDatabase,
		SystemRouterDb::kNodesTable) {
}

SystemRouterDataProviderDb::~SystemRouterDataProviderDb() = default;
