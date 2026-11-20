#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_model.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_role.h"

#include <QDataStream>
#include <QIODevice>
#include <QMimeData>

namespace {

bool isServiceItem(QStandardItem* item) {
	if (!item) {
		return false;
	}

	const auto state = LazyTreeNodeRole::nodeStateByItem(item);
	return state && (state->dummy() || state->temporary());
}

} // namespace

LazyTreeModel::LazyTreeModel(QObject* parent)
	: QStandardItemModel(parent) {
}

QMimeData* LazyTreeModel::mimeData(const QModelIndexList& indexes) const {
	for (const QModelIndex& index : indexes) {
		if (!index.isValid() || index.column() != 0) {
			continue;
		}

		auto* item = itemFromIndex(index);
		if (!item || isServiceItem(item)) {
			continue;
		}

		const auto node = LazyTreeNodeRole::nodeByItem(item);
		if (!node) {
			continue;
		}

		QByteArray encodedData;
		QDataStream stream(&encodedData, QIODevice::WriteOnly);
		stream << node->id();

		auto* mimeData = new QMimeData();
		mimeData->setData(MimeType, encodedData);
		return mimeData;
	}

	return nullptr;
}

QStringList LazyTreeModel::mimeTypes() const {
	return { MimeType };
}

Qt::DropActions LazyTreeModel::supportedDropActions() const {
	return Qt::MoveAction;
}

bool LazyTreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
	int row, int column, const QModelIndex& parent) const {
	Q_UNUSED(row);
	Q_UNUSED(column);

	if (action != Qt::MoveAction || !data || !data->hasFormat(MimeType)) {
		return false;
	}

	QByteArray encodedData = data->data(MimeType);
	QDataStream stream(&encodedData, QIODevice::ReadOnly);
	QVariant draggedId;
	stream >> draggedId;

	auto* draggedItem = findItemById(draggedId);
	if (!draggedItem) {
		return false;
	}

	auto* targetParent = itemFromIndex(parent);
	if (!targetParent) {
		targetParent = invisibleRootItem();
	}

	if (isServiceItem(targetParent)) {
		return false;
	}

	if (const auto state = LazyTreeNodeRole::nodeStateByItem(targetParent);
		state && state->needFetchChildren()) {
		// Дети узла ещё не загружены: позиция вставки неизвестна.
		return false;
	}

	return !isDescendantOf(draggedItem, targetParent);
}

bool LazyTreeModel::dropMimeData(const QMimeData* data, Qt::DropAction action,
	int row, int column, const QModelIndex& parent) {
	Q_UNUSED(data);
	Q_UNUSED(action);
	Q_UNUSED(row);
	Q_UNUSED(column);
	Q_UNUSED(parent);

	// Перемещение выполняет LazyTreeView::dropEvent: сначала сохраняется
	// новый родитель, затем элемент переносится в модели.
	return false;
}

QStandardItem* LazyTreeModel::findItemById(const QVariant& id) const {
	if (!id.isValid() || id.isNull()) {
		return nullptr;
	}

	QList<QStandardItem*> stack;
	for (int i = 0; i < invisibleRootItem()->rowCount(); i++) {
		stack.append(invisibleRootItem()->child(i));
	}

	while (!stack.isEmpty()) {
		auto* current = stack.takeFirst();
		if (!current) {
			continue;
		}

		const auto state = LazyTreeNodeRole::nodeStateByItem(current);
		if (!state || state->dummy()) {
			continue;
		}

		const auto node = LazyTreeNodeRole::nodeByItem(current);
		if (node && node->id() == id) {
			return current;
		}

		for (int i = 0; i < current->rowCount(); i++) {
			stack.append(current->child(i));
		}
	}

	return nullptr;
}

bool LazyTreeModel::isDescendantOf(QStandardItem* node, QStandardItem* target) const {
	auto* current = target;
	while (current) {
		if (current == node) {
			return true;
		}

		current = current->parent();
	}

	return false;
}
