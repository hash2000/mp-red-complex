#include "Content/SystemRouter/data_providers/system_router_data_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_db.h"
#include "Content/SystemRouter/models/system_router_data_node.h"

SystemRouterDataProviderDb::SystemRouterDataProviderDb(DatabasesService* databasesService)
	: LazyNodesDataProviderDb(databasesService, SystemRouterDb::kDatabase,
		SystemRouterDb::kNodesTable) {
}

SystemRouterDataProviderDb::~SystemRouterDataProviderDb() = default;

LazyTreeNodePtr SystemRouterDataProviderDb::createTreeNode() {
	return std::make_shared<SystemRouterDataNode>();
}

LazyTreeNodePtr SystemRouterDataProviderDb::createRootNode() {
	return std::make_shared<SystemRouterDataNode>();
}
