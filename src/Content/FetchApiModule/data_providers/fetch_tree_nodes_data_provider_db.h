#pragma once
#include "Content/FetchApiModule/data_providers/i_fetch_tree_nodes_data_provider.h"
#include <memory>

class DatabasesService;

class FetchTreeNodesDataProviderDb : public IFetchTreeNodesDataProvider {
public:
	FetchTreeNodesDataProviderDb(DatabasesService* databasesService);
	~FetchTreeNodesDataProviderDb() override;

	LazyTreeNodeList treeNodes(const QVariant& parentId) const override;
	LazyTreeNodeList searchTreeNodes(const QString& text) const override;

	bool addNode(const LazyTreeNodePtr& node) override;
	bool deleteTreeNode(const QVariant& id) override;
	bool updateTreeNode(const LazyTreeNodePtr& node) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
