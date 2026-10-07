#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_role.h"
#include <QVariant>


namespace LazyTreeNodeRoleNs {
constexpr int RawData = Qt::UserRole + 1;
constexpr int State = Qt::UserRole + 2;
}


LazyTreeNodePtr LazyTreeNodeRole::nodeByItem(QStandardItem* item) {
	return item->data(LazyTreeNodeRoleNs::RawData)
		.value<LazyTreeNodePtr>();
}

LazyTreeNodeStatePtr LazyTreeNodeRole::nodeStateByItem(QStandardItem* item) {
	return item->data(LazyTreeNodeRoleNs::State)
		.value<LazyTreeNodeStatePtr>();
}

void LazyTreeNodeRole::updateNodeRaw(QStandardItem* item, LazyTreeNodePtr node) {
	item->setData(QVariant::fromValue(node), LazyTreeNodeRoleNs::RawData);
}

void LazyTreeNodeRole::updateNodeState(QStandardItem* item, LazyTreeNodeStatePtr nodeState) {
	item->setData(QVariant::fromValue(nodeState), LazyTreeNodeRoleNs::State);
}
