#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_model.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_state.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_role.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

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
	ILazyNodesDataProvider* dataProvider = nullptr;
	LazyTreeNodePtr rootNode;

	void buildSubTree(QStandardItem* parentItem, const LazyTreeNodeList& nodes);
	void addDummyNode(QStandardItem* parentItem);
	bool isDummyNode(QStandardItem* item) const;
	bool isTemporaryNode(QStandardItem* item) const;
	void preareRootItemModel(QStandardItemModel* model);
	void removeTemporaryNode(QStandardItem* item);
	void checkCurrentItemAndRemoveIfTemporary();
	void finalizeTemporaryNode(QStandardItem* item);
	bool searchAndRemoveDummyNode(QStandardItem* parent);
	void removeAllChildren(QStandardItem* parentItem);
};

LazyTreeView::LazyTreeView(ILazyNodesDataProvider* provider, QWidget* parent)
	: d(std::make_unique<Private>(this)) {
	d->model = new LazyTreeModel(this);
	d->dataProvider = provider;
	d->rootNode = provider->createRootNode();
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

	// Получаем ID перетаскиваемого узла из MIME-данных
	QStandardItem* targetItem = d->model->itemFromIndex(targetIndex);
	if (!targetItem || d->isDummyNode(targetItem)) {
		event->ignore();
		return;
	}

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

	auto oldParent = draggedItem->parent();
	if (!oldParent) {
		oldParent = d->model->invisibleRootItem();
	}

	auto oldParentIndex = oldParent->index();

	// Определяем нового родителя (с учетом позиции drop)
	auto newParent = targetItem;
	auto dropPos = dropIndicatorPosition();
	if (dropPos == QAbstractItemView::AboveItem || dropPos == QAbstractItemView::BelowItem) {
		newParent = targetItem->parent();
		if (!newParent) {
			newParent = d->model->invisibleRootItem();
		}
	}

	// Если всё успешно, разрешаем событию завершиться
	QTreeView::dropEvent(event);

	auto draggedNode = LazyTreeNodeRole::nodeByItem(draggedItem);
	auto newParentNode = LazyTreeNodeRole::nodeByItem(newParent);
	auto newParentId = newParentNode->id();
	auto newParentIndex = newParent->index();
	auto isRoot = (newParent == d->model->invisibleRootItem());

	draggedNode->setParentId(newParentId);

	// Вызываем ActionHandler для сохранения в БД
	if (!d->dataProvider) {
		event->ignore();
		return;
	}

	auto result = d->dataProvider->updateTreeNode(draggedNode);
	if (!result) {
		qWarning() << "LazyTreeView::dropEvent: dragged" << draggedItem->text()
			<< "from" << oldParent->text()
			<< "to" << newParent->text();
		// showError(result.errorMessage);
		event->ignore();
		return;
	}

	// дял рутового узла QTreeView не работают методы collapse/expand
	// его нужно обрабатывать отдельно
	if (isRoot) {
		d->removeAllChildren(newParent);
		emit requestFetchChildren(QVariant());
	}
	else {
		collapse(newParentIndex);
		d->removeAllChildren(newParent);
		d->addDummyNode(newParent);
		expand(newParentIndex);
	}

	// Эмитим сигнал после полного завершения операции
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
		auto nodeState = std::make_shared<LazyTreeNodeState>();
		const auto& children = nodeData->children();
		LazyTreeNodeRole::updateNodeState(item, nodeState);
		LazyTreeNodeRole::updateNodeRaw(item, nodeData);
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
	auto dummy = new QStandardItem("Loading...");
	auto nodeState = std::make_shared<LazyTreeNodeState>();
	nodeState->setDummy(true);
	LazyTreeNodeRole::updateNodeState(dummy, nodeState);
	dummy->setFlags(Qt::NoItemFlags); // Неактивный, не кликабельный
	parentItem->appendRow(dummy);
}

bool LazyTreeView::Private::isDummyNode(QStandardItem* item) const {
	return item && LazyTreeNodeRole::nodeStateByItem(item)->dummy();
}

bool LazyTreeView::Private::isTemporaryNode(QStandardItem* item) const {
	return item && LazyTreeNodeRole::nodeStateByItem(item)->temporary();
}

void LazyTreeView::onExpanded(const QModelIndex& index) {
	qDebug() << "LazyTreeView::onExpanded";
	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item) {
		return;
	}

	// Если это первый раз и там dummy-узел
	if (d->searchAndRemoveDummyNode(item)) {
		const auto parentId = LazyTreeNodeRole::nodeByItem(item)->id();
		emit requestFetchChildren(parentId);
	}
}

bool LazyTreeView::Private::searchAndRemoveDummyNode(QStandardItem* parent) {
	auto rows = parent->rowCount();
	for (int i = 0; i < rows; i++) {
		if (isDummyNode(parent->child(i))) {
			parent->removeRow(i);
			return true;
		}
	}

	return false;
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
	// Рекурсивный поиск по всей модели/
	QList<QStandardItem*> stack;
	stack.append(d->model->invisibleRootItem());

	while (!stack.isEmpty()) {
		const auto current = stack.takeFirst();
		const auto nodeState = LazyTreeNodeRole::nodeStateByItem(current);
		if (nodeState->dummy()) {
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

QStandardItem* LazyTreeView::itemFromIndex(const QModelIndex& index) const {
	return d->model->itemFromIndex(index);
}

void LazyTreeView::removeSelectedNode() {
	if (!d->dataProvider) {
	//	showError("Обработчик действий не установлен");
		return;
	}

	QModelIndex currentIndex = this->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(currentIndex);
	if (!item || d->isTemporaryNode(item)) {
		return;
	}

	const auto node = LazyTreeNodeRole::nodeByItem(item);
	const auto nodeId = node->id();

	auto result = d->dataProvider->deleteTreeNode(nodeId);
	if (!result) {
		// showError(result.errorMessage);
		return;
	}

	auto parent = item->parent();
	if (parent) {
		parent->removeRow(item->row());
	}
	else {
		d->model->removeRow(item->row());
	}

	emit nodeRemoved(node);
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

	return LazyTreeNodeRole::nodeByItem(item);
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

	return LazyTreeNodeRole::nodeByItem(item)->id();
}


void LazyTreeView::startEditSelectedNode() {
	startEdit(currentIndex());
}

void LazyTreeView::startEdit(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	auto item = itemFromIndex(index);

	Qt::ItemFlags flags = item->flags();
	flags &= ~Qt::ItemIsDragEnabled;
	flags &= ~Qt::ItemIsDropEnabled;
	flags |= Qt::ItemIsEditable;
	item->setFlags(flags);

	setCurrentIndex(index);
	edit(index);
}

bool LazyTreeView::addNodeAndStartEdit(QStandardItem* parentItem, LazyTreeNodePtr newNode) {
	if (!parentItem) {
		parentItem = d->model->invisibleRootItem();
	}

	if (d->isTemporaryNode(parentItem)) {
		qWarning() << "LazyTreeView::addNodeAndStartEdit: allready is temporary" << parentItem->text();
		return false;
	}

	auto item = new QStandardItem("");
	auto nodeState = std::make_shared<LazyTreeNodeState>();
	nodeState->setTemporary(true);

	LazyTreeNodeRole::updateNodeState(item, nodeState);
	LazyTreeNodeRole::updateNodeRaw(item, newNode);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

	parentItem->appendRow(item);

	auto index = d->model->indexFromItem(item);

	d->lastCommittedText = QString();
	setExpanded(d->model->indexFromItem(parentItem), true);

	// reindex
	index = item->index();

	setCurrentIndex(index);
	edit(index);
	return true;
}

void LazyTreeView::onSelectionChanged(const QModelIndex& current, const QModelIndex& previous) {
	if (!current.isValid()) {
		return;
	}

	// выбор item по индексу должен быть обязательно до проверки previous
	// иначе после finalizeEditItem индекс будет указывать на другую ветку
	// и item будет указывать не на тот узел
	auto item = d->model->itemFromIndex(current);
	auto nodeState = LazyTreeNodeRole::nodeStateByItem(item);
	if (!nodeState) {
		qWarning() << "LazyTreeView::onSelectionChanged: item " << item->text() << "is'nt contains NodeState";
		return;
	}

	if (previous.isValid()) {
		auto prevItem = d->model->itemFromIndex(previous);
		auto prevNodeState = LazyTreeNodeRole::nodeStateByItem(prevItem);
		if (prevNodeState->temporary()) {
			finalizeEditItem(prevItem);
		}
	}

	auto isDummy = nodeState->dummy();
	auto isTemp = nodeState->temporary();
	auto selectedNode = LazyTreeNodePtr();

	if (item && !isDummy && !isTemp) {
		selectedNode = LazyTreeNodeRole::nodeByItem(item);
	}

	emit selectionChanged(selectedNode);
}

void LazyTreeView::onItemChanged(QStandardItem* item) {
	finalizeEditItem(item);
}

void LazyTreeView::finalizeEditItem(QStandardItem* item) {
	if (!d->dataProvider) {
		//	showError("Обработчик действий не установлен");
		return;
	}

	QString currentText = item->text().trimmed();
	if (currentText == d->lastCommittedText && !currentText.isEmpty()) {
		return;
	}

	d->lastCommittedText = currentText;

	auto nodeState = LazyTreeNodeRole::nodeStateByItem(item);
	if (!nodeState) {
		qWarning() << "LazyTreeView::onSelectionChanged: item " << item->text() << "is'nt contains NodeState";
		return;
	}

	auto node = LazyTreeNodeRole::nodeByItem(item);

	if (nodeState->temporary()) {
		if (currentText.isEmpty()) {
			d->removeTemporaryNode(item);
			return;
		}

		node->setName(currentText);

		QStandardItem* parentItem = item->parent();
		if (!parentItem) {
			parentItem = d->model->invisibleRootItem();
		}

		auto parentNode = LazyTreeNodeRole::nodeByItem(parentItem);
		node->setParentId(parentNode->id());

		auto result = d->dataProvider->addTreeNode(node);

		if (result) {
			d->finalizeTemporaryNode(item);
			emit nodeAdded(node);
		}
		else {
			// showError(result.errorMessage);
			d->removeTemporaryNode(item);
		}
	}
	else {
		if (currentText.isEmpty()) {
			auto nodeData = LazyTreeNodeRole::nodeByItem(item);
			item->setText(nodeData->name());
			d->lastCommittedText = nodeData->name();
			// showError("Имя не может быть пустым");
		}
		else {

			auto prevName = node->name();
			node->setName(currentText);
			auto result = d->dataProvider->updateTreeNode(node);

			if (result) {
				emit nodeRenamed(node, currentText);
			}
			else {
				// Откатываем изменение
				auto nodeData = LazyTreeNodeRole::nodeByItem(item);
				item->setText(prevName);
				d->lastCommittedText = prevName;
				//showError(result.errorMessage);
			}
		}
	}

	d->finalizeTemporaryNode(item);
}

void LazyTreeView::Private::finalizeTemporaryNode(QStandardItem* item) {
	auto nodeState = LazyTreeNodeRole::nodeStateByItem(item);
	auto node = LazyTreeNodeRole::nodeByItem(item);

	nodeState->setTemporary(false);

	Qt::ItemFlags flags = item->flags();
	flags &= ~Qt::ItemIsEditable;
	flags |= Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
	item->setFlags(flags);
}

void LazyTreeView::onNodeActivated(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item) {
		return;
	}

	auto nodeStates = LazyTreeNodeRole::nodeStateByItem(item);
	if (nodeStates->dummy()) {
		return;
	}

	auto node = LazyTreeNodeRole::nodeByItem(item);
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
	auto nodeState = std::make_shared<LazyTreeNodeState>();
	LazyTreeNodeRole::updateNodeState(normalRootItem, nodeState);
	LazyTreeNodeRole::updateNodeRaw(normalRootItem, rootNode);
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
	auto currentIndex = q->currentIndex();
	if (!currentIndex.isValid()) {
		return;
	}

	auto item = model->itemFromIndex(currentIndex);
	if (!item) {
		return;
	}

	removeTemporaryNode(item);
}

void LazyTreeView::Private::removeAllChildren(QStandardItem* parentItem) {
	if (!parentItem) {
		return;
	}

	auto count = parentItem->rowCount();
	if (count > 0) {
		parentItem->removeRows(0, count);
	}
}
