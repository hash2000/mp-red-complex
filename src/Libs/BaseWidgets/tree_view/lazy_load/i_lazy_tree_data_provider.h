#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QString>
#include <QVariant>


class ILazyNodesDataProvider {
public:
	virtual ~ILazyNodesDataProvider() = default;
	virtual LazyTreeNodePtr createTreeNode() = 0;
	virtual LazyTreeNodeList treeNodes(const QVariant& parentId) const = 0;
	virtual LazyTreeNodeList searchTreeNodes(const QString& text) const = 0;
	virtual bool addTreeNode(const LazyTreeNodePtr& node) = 0;
	virtual bool deleteTreeNode(const QVariant& id) = 0;
	virtual bool updateTreeNode(const LazyTreeNodePtr& node) = 0;
};
