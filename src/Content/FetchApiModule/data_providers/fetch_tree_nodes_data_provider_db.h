#pragma once
#include "Content/FetchApiModule/data_providers/i_fetch_tree_nodes_data_provider.h"
#include <memory>

class DatabasesService;

class FetchTreeNodesDataProviderDb : public IFetchTreeNodesDataProvider {
public:
	FetchTreeNodesDataProviderDb(DatabasesService* databasesService);
	~FetchTreeNodesDataProviderDb() override;

	std::list<FetchTree> treeNodes(std::optional<int> parentId) override;
	bool deleteTreeNode(int id) override;
	bool updateTreeNode(const FetchTree& node) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
