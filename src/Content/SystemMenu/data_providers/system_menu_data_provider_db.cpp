#include "Content/SystemMenu/data_providers/system_menu_data_provider_db.h"

namespace {
const QString kDatabaseName = QStringLiteral("system_menu");
const QString kTableName = QStringLiteral("menu_items");
}

SystemMenuDataProviderDb::SystemMenuDataProviderDb(DatabasesService* databasesService)
	: LazyNodesDataProviderDb(databasesService, kDatabaseName, kTableName) {
}

SystemMenuDataProviderDb::~SystemMenuDataProviderDb() = default;
