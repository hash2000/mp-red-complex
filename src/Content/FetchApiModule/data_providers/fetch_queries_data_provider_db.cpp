#include "Content/FetchApiModule/data_providers/fetch_queries_data_provider_db.h"

FetchQueriesDataProviderDb::FetchQueriesDataProviderDb(DatabasesService* databasesService)
	: LazyNodesDataProviderDb(databasesService, QStringLiteral("fetch_api"), QStringLiteral("queries")) {
}

FetchQueriesDataProviderDb::~FetchQueriesDataProviderDb() = default;
