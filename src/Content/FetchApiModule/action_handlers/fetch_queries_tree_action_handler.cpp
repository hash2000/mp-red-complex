#include "Content/FetchApiModule/action_handlers/fetch_queries_tree_action_handler.h"
#include "Content/FetchApiModule/data_providers/fetch_tree_nodes_data_provider_db.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_roles.h"

#include <QVariant>
#include <QMessageBox>

using ActionResult = ILazyTreeNodeActionHandler::ActionResult;

LazyTreeNodeActionHandler::LazyTreeNodeActionHandler(ILazyNodesDataProvider* provider, QWidget* parent)
	: _provider(provider)
	, _parent(parent) {
}

ActionResult LazyTreeNodeActionHandler::addNode(QStandardItem* parentItem,  LazyTreeNodePtr newNode) {
	auto parentId = parentItem->data(LazyTreeNodeRole::Id);
	newNode->setParentId(parentId);

	if (!_provider->addTreeNode(newNode)) {
		return ActionResult::Error("Не удалось сохранить узел в базу данных");
	}

	return ActionResult::Ok();
}

ActionResult LazyTreeNodeActionHandler::removeNode(QStandardItem* nodeItem) {
	QVariant nodeId = nodeItem->data(LazyTreeNodeRole::Id);
	if (!_provider->deleteTreeNode(nodeId.toInt())) {
		return ActionResult::Error("Не удалось удалить узел из базы данных");
	}
	return ActionResult::Ok();
}

ActionResult LazyTreeNodeActionHandler::renameNode(QStandardItem* nodeItem, const QString& newName) {
	auto nodeData = nodeItem->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();

	// Валидация уникальности среди братьев
	QStandardItem* parent = nodeItem->parent();
	if (!parent) {
		parent = nodeItem->model()->invisibleRootItem();
	}

	nodeData->setName(newName);
	if (!_provider->updateTreeNode(nodeData)) {
		return ActionResult::Error("Не удалось обновить узел в базе данных");
	}

	return ActionResult::Ok();
}

ActionResult LazyTreeNodeActionHandler::moveNode(QStandardItem* nodeItem, QStandardItem* newParentItem) {
	auto nodeData = nodeItem->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();

	QVariant newParentId = newParentItem->data(LazyTreeNodeRole::RawData);
	nodeData->setParentId(newParentId);

	if (!_provider->updateTreeNode(nodeData)) {
		return ActionResult::Error("Не удалось переместить узел");
	}

	return ActionResult::Ok();
}
