#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include <QString>

#include <optional>
#include <vector>

class FetchTree;

class IFetchTreeNodesDataProvider {
public:
	virtual ~IFetchTreeNodesDataProvider() = default;

	virtual LazyTreeNodeList treeNodes(const QVariant& parentId) const = 0;
	virtual LazyTreeNodeList searchTreeNodes(const QString& text) const = 0;

	virtual bool addNode(const LazyTreeNodePtr& node) = 0;
	virtual bool deleteNode(const QVariant& id) = 0;
	virtual bool updateNode(const LazyTreeNodePtr& node) = 0;
};
