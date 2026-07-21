#include "Content/FetchApiModule/data_providers/fetch_tree_nodes_data_provider_db.h"
#include "Content/FetchApiModule/models/fetch_tree.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

class FetchTreeNodesDataProviderDb::Private {
public:
	Private(FetchTreeNodesDataProviderDb* paren) : q(paren) {}
	FetchTreeNodesDataProviderDb* q;

	DatabasesService* databasesService;
};

FetchTreeNodesDataProviderDb::FetchTreeNodesDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
}

FetchTreeNodesDataProviderDb::~FetchTreeNodesDataProviderDb() = default;

std::list<FetchTree> FetchTreeNodesDataProviderDb::treeNodes(std::optional<int> parentId) {

	return std::list<FetchTree>();
}

bool FetchTreeNodesDataProviderDb::deleteTreeNode(int id) {

	return true;
}

bool FetchTreeNodesDataProviderDb::updateTreeNode(const FetchTree& node) {

	return true;
}

