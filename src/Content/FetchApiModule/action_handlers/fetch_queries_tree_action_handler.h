#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node_action_handler.h"

#include <QWidget>

class ILazyNodesDataProvider;

class LazyTreeNodeActionHandler : public ILazyTreeNodeActionHandler {
public:
	LazyTreeNodeActionHandler(ILazyNodesDataProvider* provider, QWidget* parent);

	ActionResult addNode(QStandardItem* parentItem, LazyTreeNodePtr newNode) override;
	ActionResult removeNode(QStandardItem* nodeItem) override;
	ActionResult renameNode(QStandardItem* nodeItem, const QString& newName) override;
	ActionResult moveNode(QStandardItem* nodeItem, QStandardItem* newParentItem) override;

private:
	ILazyNodesDataProvider* _provider;
	QWidget* _parent;
};

