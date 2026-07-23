#pragma once
#include <QString>

#include <optional>

class FetchQuery;

class IFetchQueriesDataProvider {
public:
	virtual ~IFetchQueriesDataProvider() = default;

	virtual std::optional<FetchQuery> query(int tree_id) = 0;
	virtual bool deleteQuery(int treeId) = 0;
	virtual bool updateQuery(const FetchQuery& query) = 0;
};
