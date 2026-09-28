#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_model.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_roles.h"

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
	qDebug() << "mimeData";
	for (const QModelIndex& index : indexes) {
		if (index.isValid()) {
			QVariant nodeId = index.data(LazyTreeNodeRole::Id);
			stream << nodeId;
			qDebug() << "** id" << nodeId;
		}
	}

	mimeData->setData(MimeType, encodedData);
	return mimeData;
}

Qt::DropActions LazyTreeModel::supportedDropActions() const {
	return Qt::MoveAction | Qt::TargetMoveAction;
}

bool LazyTreeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action,
	int row, int column, const QModelIndex& parent) const {
	Q_UNUSED(action);
	Q_UNUSED(row);
	Q_UNUSED(column);
	Q_UNUSED(parent);

	// Разрешаем drop, если это наш формат данных
	auto result =  data->hasFormat(MimeType);
	if (!result) {
		qDebug() << "canDropMimeData" << result;
	}

	return result;
}

bool LazyTreeModel::moveRows(const QModelIndex& sourceParent, int sourceRow, int count,
	const QModelIndex& destinationParent, int destinationChild) {

	// Валидация параметров
	if (sourceRow < 0 || count <= 0 || destinationChild < 0) {
		return false;
	}

	// Получаем родителей
	QStandardItem* sourceParentItem = itemFromIndex(sourceParent);
	if (!sourceParentItem) {
		sourceParentItem = invisibleRootItem();
	}

	QStandardItem* destParentItem = itemFromIndex(destinationParent);
	if (!destParentItem) {
		destParentItem = invisibleRootItem();
	}

	// Проверка: нельзя перемещать в потомка
	if (isDescendantOf(destParentItem, sourceParentItem)) {
		return false;
	}

	if (!beginMoveRows(sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild)) {
		return false;
	}

	// Корректируем destinationChild, если перемещаем внутри одного родителя
	int adjustedDestChild = destinationChild;
	if (sourceParentItem == destParentItem && sourceRow < destinationChild) {
		adjustedDestChild -= count;
	}

	// Извлекаем строки из источника
	QList< QList<QStandardItem*>> takenRows;
	for (int i = 0; i < count; i++) {
		auto itm = sourceParentItem->takeRow(sourceRow + i);
		if (itm.isEmpty()) {
			continue;
		}
		takenRows.append(itm);
	}

	// Вставляем в destination
	for (int i = 0; i < takenRows.count() - 1; i++) {
		const auto itm = takenRows[i];
		destParentItem->insertRow(adjustedDestChild + i, itm);
	}

	endMoveRows();

	return true;
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

	// Исходная позиция
	QModelIndex sourceParentIndex;
	int sourceRow = draggedItem->row();
	QStandardItem* oldParent = draggedItem->parent();
	if (oldParent && oldParent != invisibleRootItem()) {
		sourceParentIndex = oldParent->index();
	}

	// Целевая позиция
	QModelIndex destParentIndex;
	int destRow = (row == -1) ? targetParent->rowCount() : row;
	if (targetParent != invisibleRootItem()) {
		destParentIndex = targetParent->index();
	}

	// moveRow вызовет наш переопределенный moveRows
	return moveRow(sourceParentIndex, sourceRow, destParentIndex, destRow);
}

QStandardItem* LazyTreeModel::findItemById(const QVariant& id) const {
	QList<QStandardItem*> stack;
	stack.append(invisibleRootItem());

	while (!stack.isEmpty()) {
		QStandardItem* current = stack.takeFirst();
		if (current->data(LazyTreeNodeRole::Id) == id) {
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
