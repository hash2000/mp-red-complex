#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_roles.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_model.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node_action_handler.h"

#include <QMouseEvent>
#include <QStandardItemModel>
#include <QVariant>
#include <QDropEvent>
#include <QMimeData>
#include <QTimer>

class LazyTreeView::Private {
public:
	Private(LazyTreeView* parent) : q(parent) {}
	LazyTreeView* q;

	QString lastCommittedText;
	LazyTreeModel* model;
	ILazyTreeNodeActionHandler* actionHandler = nullptr;

	void buildSubTree(QStandardItem* parentItem, const LazyTreeNodeList& nodes);
	void addDummyNode(QStandardItem* parentItem);
	bool isDummyNode(QStandardItem* item) const;
	bool isTemporaryNode(QStandardItem* item) const;
	void preareRootItemModel(QStandardItemModel* model);
	void finalizeTemporaryNode(QStandardItem* item);
	void removeTemporaryNode(QStandardItem* item);
	void checkCurrentItemAndRemoveIfTemporary();
};

LazyTreeView::LazyTreeView(QWidget* parent)
	: d(std::make_unique<Private>(this)) {
	d->model = new LazyTreeModel(this);
	setModel(d->model);
	setHeaderHidden(true);
	setUniformRowHeights(true); // Оптимизация для больших деревьев
	setSelectionMode(QAbstractItemView::SingleSelection);
	setIndentation(10);

	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setDragDropMode(QAbstractItemView::InternalMove);

	connect(this, &QTreeView::expanded, this, &LazyTreeView::onExpanded);
	connect(this, &QTreeView::activated, this, &LazyTreeView::onNodeActivated);
	connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &LazyTreeView::onSelectionChanged);
	connect(d->model, &QStandardItemModel::itemChanged, this, &LazyTreeView::onItemChanged);
}

LazyTreeView::~LazyTreeView() = default;

void LazyTreeView::setActionHandler(ILazyTreeNodeActionHandler* actionHandler) {
	d->actionHandler = actionHandler;
}

void LazyTreeView::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		QModelIndex index = indexAt(event->pos());
		if (!index.isValid()) {
			setCurrentIndex(QModelIndex());
			clearSelection();
			emit emptyAreaClicked();
			emit selectionChanged(LazyTreeNodePtr());
			return;
		}
	}

	QTreeView::mousePressEvent(event);
}

void LazyTreeView::dropEvent(QDropEvent* event) {
	QModelIndex targetIndex = indexAt(event->position().toPoint());
	if (!targetIndex.isValid()) {
		event->ignore();
		return;
	}

	QStandardItem* targetItem = d->model->itemFromIndex(targetIndex);
	if (!targetItem || d->isDummyNode(targetItem)) {
		event->ignore();
		return;
	}

	// Получаем ID перетаскиваемого узла из MIME-данных
	if (!event->mimeData()->hasFormat(LazyTreeModel::MimeType)) {
		event->ignore();
		return;
	}

	QByteArray encodedData = event->mimeData()->data(LazyTreeModel::MimeType);
	QDataStream stream(&encodedData, QIODevice::ReadOnly);
	QVariant draggedId;
	stream >> draggedId;

	QStandardItem* draggedItem = d->model->findItemById(draggedId);
	if (!draggedItem) {
		event->ignore();
		return;
	}

	// Определяем нового родителя (с учетом позиции drop)
	auto newParent = targetItem;
	auto dropPos = dropIndicatorPosition();
	if (dropPos == QAbstractItemView::AboveItem || dropPos == QAbstractItemView::BelowItem) {
		newParent = targetItem->parent();
		if (!newParent) {
			newParent = d->model->invisibleRootItem();
		}
	}

	// Вызываем ActionHandler для сохранения в БД
	if (d->actionHandler) {
		auto result = d->actionHandler->moveNode(draggedItem, newParent);
		if (!result.success) {
			// showError(result.errorMessage);
			event->ignore();
			return;
		}
	}

	// Если всё успешно, разрешаем событию завершиться
	QTreeView::dropEvent(event);

	QModelIndex newParentIndex = newParent->index();
	if (newParentIndex.isValid()) {
		setExpanded(newParentIndex, true);
	}

	// Эмитим сигнал после полного завершения операции
	auto draggedNode = draggedItem->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();
	auto newParentId = newParent->data(LazyTreeNodeRole::Id);
	emit nodeMoved(draggedNode, newParentId);
}

void LazyTreeView::setNodes(const LazyTreeNodeList& nodes) {
	d->model->clear();
	d->buildSubTree(d->model->invisibleRootItem(), nodes);
	d->preareRootItemModel(d->model);
}

void LazyTreeView::Private::buildSubTree(QStandardItem* parentItem, const LazyTreeNodeList& nodes) {
	for (const auto& nodeData : nodes) {
		auto item = new QStandardItem(nodeData->name());
		const auto& children = nodeData->children();
		item->setData(nodeData->id(), LazyTreeNodeRole::Id);
		item->setData(QVariant::fromValue(nodeData), LazyTreeNodeRole::RawData);
		item->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsSelectable |
			Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

		parentItem->appendRow(item);

		if (children.has_value()) {
			if (!children->empty()) {
				buildSubTree(item, *children);
				if (nodeData->expanded()) {
					q->expand(item->index());
				}
			}
			else {
				addDummyNode(item);
			}
		}
	}
}

void LazyTreeView::Private::addDummyNode(QStandardItem* parentItem) {
	auto dummy = new QStandardItem("Loading..."); // Можно оставить пустым ""
	dummy->setData(true, LazyTreeNodeRole::IsDummy);
	dummy->setFlags(Qt::NoItemFlags); // Неактивный, не кликабельный
	parentItem->appendRow(dummy);
}

bool LazyTreeView::Private::isDummyNode(QStandardItem* item) const {
	return item && item->data(LazyTreeNodeRole::IsDummy).toBool();
}

bool LazyTreeView::Private::isTemporaryNode(QStandardItem* item) const {
	return item && item->data(LazyTreeNodeRole::IsTemp).toBool();
}

void LazyTreeView::onExpanded(const QModelIndex& index) {
	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item) {
		return;
	}

	// Если это первый раз и там dummy-узел
	if (item->rowCount() == 1 && d->isDummyNode(item->child(0))) {
		QVariant parentId = item->data(LazyTreeNodeRole::Id);
		item->removeRow(0);
		emit requestFetchChildren(parentId);
	}
}

void LazyTreeView::appendChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes) {
	QStandardItem* parentItem = findNode(parentId);
	if (!parentItem) {
		return;
	}

	if (newNodes.empty()) {
		// Если детей нет, просто оставляем узел пустым (он уже развернут, dummy удален)
		return;
	}

	d->buildSubTree(parentItem, newNodes);
}

QStandardItem* LazyTreeView::findNode(const QVariant& id) const {
	// Рекурсивный поиск по всей модели
	QList<QStandardItem*> stack;
	stack.append(d->model->invisibleRootItem());

	while (!stack.isEmpty()) {
		auto current = stack.takeFirst();

		if (current->data(LazyTreeNodeRole::Id) == id) {
			return current;
		}

		for (int i = 0; i < current->rowCount(); i++) {
			stack.append(current->child(i));
		}
	}

	return nullptr;
}

QStandardItem* LazyTreeView::itemFromIndex(const QModelIndex& index) const {
	return d->model->itemFromIndex(index);
}

void LazyTreeView::removeSelectedNode() {
	QModelIndex currentIndex = this->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(currentIndex);
	if (!item || d->isTemporaryNode(item)) {
		return;
	}

	if (!d->actionHandler) {
	//	showError("Обработчик действий не установлен");
		return;
	}

	auto result = d->actionHandler->removeNode(item);

	if (result.success) {
		auto nodeId = item->data(LazyTreeNodeRole::Id);
		auto parent = item->parent();
		auto node = item->data(LazyTreeNodeRole::RawData)
			.value<LazyTreeNodePtr>();

		if (parent) {
			parent->removeRow(item->row());
		}
		else {
			d->model->removeRow(item->row());
		}

		emit nodeRemoved(node);
	}
	else {
		// showError(result.errorMessage);
	}
}

LazyTreeNodePtr LazyTreeView::selectedNode() const {
	QModelIndex currentIndex = this->currentIndex();
	if (!currentIndex.isValid()) {
		return LazyTreeNodePtr();
	}

	QStandardItem* item = d->model->itemFromIndex(currentIndex);
	if (!item || d->isTemporaryNode(item)) {
		return LazyTreeNodePtr();
	}

	return item->data(LazyTreeNodeRole::RawData)
		.value<LazyTreeNodePtr>();
}

QVariant LazyTreeView::selectedNodeId() const {
	QModelIndex currentIndex = this->currentIndex();
	if (!currentIndex.isValid()) {
		return QVariant();
	}

	QStandardItem* item = d->model->itemFromIndex(currentIndex);
	if (!item || d->isTemporaryNode(item)) {
		return QVariant();
	}

	return item->data(LazyTreeNodeRole::Id);
}

void LazyTreeView::startEdit(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	d->model->blockSignals(true);

	auto item = itemFromIndex(index);

	Qt::ItemFlags flags = item->flags();
	flags &= ~Qt::ItemIsDragEnabled;
	flags &= ~Qt::ItemIsDropEnabled;
	flags |= Qt::ItemIsEditable;
	item->setFlags(flags);

	d->model->blockSignals(false);

	edit(index);
}

void LazyTreeView::startEditSelectedNode() {
	startEdit(currentIndex());
}

void LazyTreeView::addNodeAndStartEdit(QStandardItem* parentItem, LazyTreeNodePtr newNode) {
	if (!parentItem) {
		parentItem = d->model->invisibleRootItem();
	}
	else if (d->isTemporaryNode(parentItem)) {
		return;
	}

	auto item = new QStandardItem("");
	item->setData(QVariant(), LazyTreeNodeRole::Id);
	item->setData(QVariant::fromValue(newNode), LazyTreeNodeRole::RawData);
	item->setData(true, LazyTreeNodeRole::IsTemp);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

	parentItem->appendRow(item);

	auto index = d->model->indexFromItem(item);

	d->lastCommittedText = QString();
	setExpanded(d->model->indexFromItem(parentItem), true);
	setCurrentIndex(index);
	edit(index);
}

void LazyTreeView::onSelectionChanged(const QModelIndex& current, const QModelIndex& previous) {
	Q_UNUSED(previous);

	if (!current.isValid()) {
		return;
	}

	auto item = d->model->itemFromIndex(current);
	auto isDummy = d->isDummyNode(item);
	auto isTemp = d->isTemporaryNode(item);

	if (item && !isDummy && !isTemp) {
		auto node = item->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();
		emit selectionChanged(node);
		return;
	}

	emit selectionChanged(LazyTreeNodePtr());
}

void LazyTreeView::onItemChanged(QStandardItem* item) {
	QString currentText = item->text();
	if (currentText == d->lastCommittedText) {
		return;
	}

	d->lastCommittedText = currentText;

	if (item->data(LazyTreeNodeRole::IsTemp).toBool()) {
		QString newName = currentText.trimmed();

		if (newName.isEmpty()) {
			d->removeTemporaryNode(item);
			return;
		}

		auto newNode = item->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();
		newNode->setName(newName);

		if (!d->actionHandler) {
			// showError("Обработчик действий не установлен");
			d->removeTemporaryNode(item);
			return;
		}

		QStandardItem* parentItem = item->parent();
		if (!parentItem) {
			parentItem = d->model->invisibleRootItem();
		}

		auto result = d->actionHandler->addNode(parentItem, newNode);

		if (result.success) {
			d->finalizeTemporaryNode(item);
			emit nodeAdded(newNode);
		}
		else {
			// showError(result.errorMessage);
			d->removeTemporaryNode(item);
		}

	}
	else if (d->actionHandler) {
		QString newName = currentText.trimmed();
		if (newName.isEmpty()) {
			auto nodeData = item->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();
			d->model->blockSignals(true);
			item->setText(nodeData->name());
			d->lastCommittedText = nodeData->name();
			d->model->blockSignals(false);
			// showError("Имя не может быть пустым");
		}
		else {
			auto result = d->actionHandler->renameNode(item, newName);
			if (result.success) {
				auto nodeData = item->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();
				nodeData->setName(newName);
				emit nodeRenamed(nodeData, newName);
			}
			else {
				// Откатываем изменение
				auto nodeData = item->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();
				d->model->blockSignals(true);
				item->setText(nodeData->name());
				d->lastCommittedText = nodeData->name();
				d->model->blockSignals(false);
				//showError(result.errorMessage);
			}
		}
	}

	d->finalizeTemporaryNode(item);
}

void LazyTreeView::onNodeActivated(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item || d->isDummyNode(item)) {
		return;
	}

	auto node = item->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();
	emit nodeActivated(node, item->text());
}

void LazyTreeView::expandAndSelectNode(const QVariant& id) {
	QStandardItem* item = findNode(id);
	if (!item) {
		return;
	}

	// Разворачиваем всех родителей
	QStandardItem* current = item->parent();
	while (current) {
		setExpanded(d->model->indexFromItem(current), true);
		current = current->parent();
	}

	// Разворачиваем и выбираем сам узел
	QModelIndex idx = d->model->indexFromItem(item);
	setExpanded(idx, true);
	scrollTo(idx, QAbstractItemView::PositionAtCenter);
	setCurrentIndex(idx);
	setFocus();
}

void LazyTreeView::Private::preareRootItemModel(QStandardItemModel* model) {
	auto normalRootItem = model->invisibleRootItem();
	normalRootItem->setData(QVariant(), LazyTreeNodeRole::Id);
	normalRootItem->setData(false, LazyTreeNodeRole::IsDummy);
}

void LazyTreeView::Private::finalizeTemporaryNode(QStandardItem* item) {
	model->blockSignals(true);

	item->setData(false, LazyTreeNodeRole::IsTemp);

	Qt::ItemFlags flags = item->flags();
	flags &= ~Qt::ItemIsEditable;
	flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
	item->setFlags(flags);

	auto newNode = item->data(LazyTreeNodeRole::RawData).value<LazyTreeNodePtr>();
	item->setData(newNode->id(), LazyTreeNodeRole::Id);

	model->blockSignals(false);
}

void LazyTreeView::Private::removeTemporaryNode(QStandardItem* item) {
	if (!item) {
		return;
	}

	if (!isTemporaryNode(item)) {
		return;
	}

	auto parent = item->parent();
	if (parent) {
		parent->removeRow(item->row());
	}
	else {
		model->removeRow(item->row());
	}
}

void LazyTreeView::Private::checkCurrentItemAndRemoveIfTemporary() {
	QModelIndex currentIndex = q->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	QStandardItem* item = model->itemFromIndex(currentIndex);
	if (!item) {
		return;
	}

	removeTemporaryNode(item);
}
