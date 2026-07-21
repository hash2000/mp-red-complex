#pragma once
#include "Content/FetchApiModule/data_providers/i_fetch_queries_data_provider.h"
#include <memory>

class DatabasesService;

class FetchQueriesDataProviderDb : public IFetchQueriesDataProvider {
public:
	FetchQueriesDataProviderDb(DatabasesService* databasesService);
	~FetchQueriesDataProviderDb() override;

	std::optional<FetchQuery> query(int tree_id) override;
	bool deleteQuery(int treeId) override;
	bool updateQuery(const FetchQuery& query) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
