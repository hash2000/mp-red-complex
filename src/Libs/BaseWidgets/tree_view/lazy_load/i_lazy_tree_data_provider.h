#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <QString>
#include <QVariant>


class ILazyNodesDataProvider {
public:
	virtual ~ILazyNodesDataProvider() = default;
	virtual LazyTreeNodePtr createTreeNode() = 0;
	virtual LazyTreeNodePtr createRootNode() = 0;
	virtual LazyTreeNodeList get(const QVariant& parentId) const = 0;
	virtual LazyTreeNodeList search(const QString& text) const = 0;
	virtual bool add(const LazyTreeNodePtr& node) = 0;
	virtual bool remove(const QVariant& id) = 0;
	virtual bool update(const LazyTreeNodePtr& node) = 0;
};
