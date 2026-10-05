#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_model.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include <QMimeData>
#include <QDataStream>
#include <QIODevice>

LazyTreeModel::LazyTreeModel(QObject* parent)
	: QStandardItemModel(parent) {
}

QMimeData* LazyTreeModel::mimeData(const QModelIndexList& indexes) const {
	QMimeData* mimeData = new QMimeData();
	QByteArray encodedData;
	QDataStream stream(&encodedData, QIODevice::WriteOnly);
	mimeData->setData(MimeType, encodedData);
	return mimeData;
}

Qt::DropActions LazyTreeModel::supportedDropActions() const {
	return Qt::MoveAction;
}

bool LazyTreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
	int row, int column, const QModelIndex& parent) const {
	return data->hasFormat(MimeType);
}

bool LazyTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
	int row, int column, const QModelIndex& parent) {
	Q_UNUSED(action);
	Q_UNUSED(column);

	if (!data->hasFormat(MimeType)) {
		return QStandardItemModel::dropMimeData(data, action, row, column, parent);
	}

	QByteArray encodedData = data->data(MimeType);
	QDataStream stream(&encodedData, QIODevice::ReadOnly);

	QVariant draggedId;
	stream >> draggedId;

	QStandardItem* draggedItem = findItemById(draggedId);
	if (!draggedItem) {
		return false;
	}

	// Определяем целевого родителя
	QStandardItem* targetParent = itemFromIndex(parent);
	if (!targetParent) {
		targetParent = invisibleRootItem();
	}

	if (isDescendantOf(draggedItem, targetParent)) {
		return false;
	}

	return true;
}

QStandardItem* LazyTreeModel::findItemById(const QVariant& id) const {
	QList<QStandardItem*> stack;
	stack.append(invisibleRootItem());

	while (!stack.isEmpty()) {
		auto current = stack.takeFirst();
		auto node = LazyTreeView::nodeByItem(current);
		if (node->id() == id) {
			return current;
		}

		for (int i = 0; i < current->rowCount(); i++) {
			stack.append(current->child(i));
		}
	}

	return nullptr;
}

bool LazyTreeModel::isDescendantOf(QStandardItem* node, QStandardItem* target) const {
	QStandardItem* current = target;
	while (current) {
		if (current == node) {
			return true;
		}

		current = current->parent();
	}

	return false;
}
