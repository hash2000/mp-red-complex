#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_state.h"

#include <QStandardItem>

class LazyTreeNodeRole {
public:
	static LazyTreeNodePtr nodeByItem(QStandardItem* item);

	static LazyTreeNodeStatePtr nodeStateByItem(QStandardItem* item);

	static void updateNodeRaw(QStandardItem* item, LazyTreeNodePtr node);

	static void updateNodeState(QStandardItem* item, LazyTreeNodeStatePtr nodeState);
};
