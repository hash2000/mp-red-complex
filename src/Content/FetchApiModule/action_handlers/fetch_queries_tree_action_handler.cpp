#include "Content/FetchApiModule/action_handlers/fetch_queries_tree_action_handler.h"
#include "Content/FetchApiModule/data_providers/fetch_tree_nodes_data_provider_db.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"

#include <QVariant>
#include <QMessageBox>

using ActionResult = ILazyTreeNodeActionHandler::ActionResult;

LazyTreeNodeActionHandler::LazyTreeNodeActionHandler(ILazyNodesDataProvider* provider,
	QWidget* parent)
	: _provider(provider)
	, _parent(parent) {
}

ActionResult LazyTreeNodeActionHandler::addNode(QStandardItem* parentItem,  LazyTreeNodePtr newNode) {
	auto node = LazyTreeView::nodeByItem(parentItem);
	newNode->setParentId(node->id());

	if (!_provider->addTreeNode(newNode)) {
		return ActionResult::Error("Не удалось сохранить узел в базу данных");
	}

	return ActionResult::Ok();
}

ActionResult LazyTreeNodeActionHandler::removeNode(QStandardItem* nodeItem) {
	auto node = LazyTreeView::nodeByItem(nodeItem);
	auto nodeId = node->id();

	if (!_provider->deleteTreeNode(nodeId.toInt())) {
		return ActionResult::Error("Не удалось удалить узел из базы данных");
	}
	return ActionResult::Ok();
}

ActionResult LazyTreeNodeActionHandler::renameNode(QStandardItem* nodeItem, const QString& newName) {
	auto node = LazyTreeView::nodeByItem(nodeItem);

	// Валидация уникальности среди братьев
	QStandardItem* parent = nodeItem->parent();
	if (!parent) {
		parent = nodeItem->model()->invisibleRootItem();
	}

	node->setName(newName);

	if (!_provider->updateTreeNode(node)) {
		return ActionResult::Error("Не удалось обновить узел в базе данных");
	}

	return ActionResult::Ok();
}

ActionResult LazyTreeNodeActionHandler::moveNode(QStandardItem* nodeItem, QStandardItem* newParentItem) {
	auto nodeData = LazyTreeView::nodeByItem(nodeItem);
	auto newParentNode = LazyTreeView::nodeByItem(newParentItem);;
	auto newParentId = newParentNode->id();

	nodeData->setParentId(newParentId);

	if (!_provider->updateTreeNode(nodeData)) {
		return ActionResult::Error("Не удалось переместить узел");
	}

	return ActionResult::Ok();
}
