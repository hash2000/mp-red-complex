#include "Content/FetchApiModule/data_providers/fetch_queries_data_provider_db.h"
#include "Content/FetchApiModule/models/fetch_query.h"
#include "Content/FetchApiModule/models/fetch_tree.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

class FetchQueriesDataProviderDb::Private {
public:
	Private(FetchQueriesDataProviderDb* paren) : q(paren) {}
	FetchQueriesDataProviderDb* q;

	DatabasesService* databasesService;
};

FetchQueriesDataProviderDb::FetchQueriesDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
}

FetchQueriesDataProviderDb::~FetchQueriesDataProviderDb() = default;

std::optional<FetchQuery> FetchQueriesDataProviderDb::query(int tree_id) {

	return std::nullopt;
}

bool FetchQueriesDataProviderDb::deleteQuery(int treeId) {

	return true;
}

bool FetchQueriesDataProviderDb::updateQuery(const FetchQuery& query) {

	return true;
}

