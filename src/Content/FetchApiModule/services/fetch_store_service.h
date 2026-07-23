#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

#include <QObject>
#include <memory>

class IFetchQueriesDataProvider;
class IFetchTreeNodesDataProvider;
class ILazyNodesDataProvider;

class FetchStoreService : public QObject, public ILazyNodesDataProvider {
public:
	FetchStoreService(
		IFetchQueriesDataProvider* queriesDataProvider,
		IFetchTreeNodesDataProvider* treeNodesDataProvider,
		QObject* parent = nullptr);

	~FetchStoreService() override;

	LazyTreeNodePtr createTreeNode() override;
	LazyTreeNodeList treeNodes(const LazyTreeNodePtr& parentNode) const override;
	LazyTreeNodeList searchTreeNodes(const QString& text) const override;
	bool addTreeNode(const LazyTreeNodePtr& node) override;
	bool deleteTreeNode(const LazyTreeNodePtr& node) override;
	bool updateTreeNode(const LazyTreeNodePtr& node) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
