#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_model.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_role.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_node_state.h"

#include <QDataStream>
#include <QDebug>
#include <QDropEvent>
#include <QIODevice>
#include <QItemSelectionModel>
#include <QMimeData>
#include <QMouseEvent>
#include <QStandardItemModel>

namespace {

/// Флаг защиты от повторного входа в завершение редактирования.
class FinalizeGuard {
public:
	explicit FinalizeGuard(bool& flag)
		: _flag(flag) {
		_flag = true;
	}

	~FinalizeGuard() {
		_flag = false;
	}

private:
	bool& _flag;
};

} // namespace

class LazyTreeView::Private {
public:
	explicit Private(LazyTreeView* parent)
		: q(parent) {
	}

	LazyTreeView* q;
	LazyTreeModel* model = nullptr;
	ILazyNodesDataProvider* provider = nullptr;
	LazyTreeNodePtr rootNode;
	QPersistentModelIndex editedIndex;
	bool finalizing = false;

	void buildSubTree(QStandardItem* parentItem, const LazyTreeNodeList& nodes);
	void addDummyNode(QStandardItem* parentItem);
	void removeDummyNodes(QStandardItem* parentItem);
	bool isDummyNode(QStandardItem* item) const;
	bool isTemporaryNode(QStandardItem* item) const;
	void prepareRootItem();
	void removeTemporaryNode(QStandardItem* item);
	void finalizeNodeEdit(QStandardItem* item);
	void finishCurrentEditing();
	bool isValidSiblingName(QStandardItem* item, const QString& name) const;
	QStandardItem* parentItemForId(const QVariant& parentId) const;
	QStandardItem* invisibleRoot() const;
	void resetItemFlags(QStandardItem* item);
	void requestChildren(QStandardItem* item);
	void removeAllChildren(QStandardItem* parentItem);
};

LazyTreeView::LazyTreeView(ILazyNodesDataProvider* provider, QWidget* parent)
	: QTreeView(parent)
	, d(std::make_unique<Private>(this)) {
	d->provider = provider;
	d->model = new LazyTreeModel(this);
	setModel(d->model);
	setHeaderHidden(true);
	setUniformRowHeights(true);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setIndentation(10);

	setDragEnabled(true);
	setAcceptDrops(true);
	setDropIndicatorShown(true);
	setDragDropMode(QAbstractItemView::InternalMove);

	d->rootNode = provider ? provider->createRootNode() : LazyTreeNodePtr();
	if (!d->rootNode) {
		d->rootNode = std::make_shared<LazyTreeDataNode>();
	}

	d->prepareRootItem();

	connect(this, &QTreeView::expanded, this, &LazyTreeView::onExpanded);
	connect(this, &QTreeView::activated, this, &LazyTreeView::onNodeActivated);
	connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &LazyTreeView::onSelectionChanged);
}

LazyTreeView::~LazyTreeView() {
	// Поставщик может быть уничтожен раньше представления: при разрушении
	// модели не должно остаться незавершённой правки, которая его использует.
	d->editedIndex = QPersistentModelIndex();
	d->provider = nullptr;
}

ILazyNodesDataProvider* LazyTreeView::provider() const {
	return d->provider;
}

void LazyTreeView::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton && !indexAt(event->pos()).isValid()) {
		// Фокус возвращается дереву: открытый редактор завершает правку.
		setFocus(Qt::MouseFocusReason);
		setCurrentIndex(QModelIndex());
		clearSelection();
		emit emptyAreaClicked();
		emit currentNodeChanged(LazyTreeNodePtr());
		return;
	}

	QTreeView::mousePressEvent(event);
}

void LazyTreeView::dropEvent(QDropEvent* event) {
	const QMimeData* mimeData = event->mimeData();
	if (!mimeData || !mimeData->hasFormat(LazyTreeModel::MimeType)) {
		QTreeView::dropEvent(event);
		return;
	}

	QByteArray encodedData = mimeData->data(LazyTreeModel::MimeType);
	QDataStream stream(&encodedData, QIODevice::ReadOnly);
	QVariant draggedId;
	stream >> draggedId;

	QStandardItem* draggedItem = d->model->findItemById(draggedId);
	if (!draggedItem || d->isDummyNode(draggedItem) || d->isTemporaryNode(draggedItem)) {
		event->ignore();
		return;
	}

	const QModelIndex targetIndex = indexAt(event->position().toPoint());
	QStandardItem* targetItem = targetIndex.isValid() ? d->model->itemFromIndex(targetIndex) : nullptr;
	if (targetItem && (d->isDummyNode(targetItem) || d->isTemporaryNode(targetItem))) {
		event->ignore();
		return;
	}

	QStandardItem* newParentItem = d->invisibleRoot();
	int row = d->invisibleRoot()->rowCount();

	if (targetItem) {
		switch (dropIndicatorPosition()) {
		case QAbstractItemView::OnItem: {
			// В недогруженный узел класть нельзя: его дети ещё неизвестны.
			const auto state = LazyTreeNodeRole::nodeStateByItem(targetItem);
			if (state && state->needFetchChildren()) {
				event->ignore();
				return;
			}

			newParentItem = targetItem;
			row = targetItem->rowCount();
			break;
		}
		case QAbstractItemView::AboveItem:
			newParentItem = targetItem->parent() ? targetItem->parent() : d->invisibleRoot();
			row = targetItem->row();
			break;
		case QAbstractItemView::BelowItem:
			newParentItem = targetItem->parent() ? targetItem->parent() : d->invisibleRoot();
			row = targetItem->row() + 1;
			break;
		default:
			break;
		}
	}

	if (!newParentItem) {
		event->ignore();
		return;
	}

	const auto newParentNode = LazyTreeNodeRole::nodeByItem(newParentItem);
	if (!newParentNode) {
		event->ignore();
		return;
	}

	if (!moveNode(draggedId, newParentNode->id(), row)) {
		event->ignore();
		return;
	}

	event->setDropAction(Qt::MoveAction);
	event->accept();
}

void LazyTreeView::closeEditor(QWidget* editor, QAbstractItemDelegate::EndEditHint hint) {
	// Запоминаем элемент до вызова базовой реализации: она может переключить
	// редактирование на соседний элемент и изменить d->editedIndex.
	const QPersistentModelIndex index = d->editedIndex;
	d->editedIndex = QPersistentModelIndex();

	QTreeView::closeEditor(editor, hint);

	if (index.isValid()) {
		if (auto* item = d->model->itemFromIndex(index)) {
			d->finalizeNodeEdit(item);
		}
	}
}

void LazyTreeView::setNodes(const LazyTreeNodeList& nodes) {
	d->editedIndex = QPersistentModelIndex();
	d->model->clear();
	d->prepareRootItem();
	d->buildSubTree(d->model->invisibleRootItem(), nodes);
}

void LazyTreeView::setChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes) {
	QStandardItem* parentItem = d->parentItemForId(parentId);
	if (!parentItem) {
		return;
	}

	d->removeAllChildren(parentItem);
	d->buildSubTree(parentItem, newNodes);

	if (const auto state = LazyTreeNodeRole::nodeStateByItem(parentItem)) {
		state->setNeedFetchChildren(false);
	}
}

void LazyTreeView::appendChildren(const QVariant& parentId, const LazyTreeNodeList& newNodes) {
	if (newNodes.empty()) {
		return;
	}

	QStandardItem* parentItem = d->parentItemForId(parentId);
	if (!parentItem) {
		return;
	}

	d->buildSubTree(parentItem, newNodes);
}

QStandardItem* LazyTreeView::findNode(const QVariant& id) const {
	return d->model->findItemById(id);
}

QStandardItem* LazyTreeView::itemFromIndex(const QModelIndex& index) const {
	return d->model->itemFromIndex(index);
}

bool LazyTreeView::isDummyItem(QStandardItem* item) const {
	return d->isDummyNode(item);
}

bool LazyTreeView::isTemporaryItem(QStandardItem* item) const {
	return d->isTemporaryNode(item);
}

void LazyTreeView::removeSelectedNode() {
	if (!d->provider) {
		return;
	}

	const QModelIndex current = currentIndex();
	if (!current.isValid()) {
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(current);
	if (!item || d->isDummyNode(item) || d->isTemporaryNode(item)) {
		return;
	}

	const auto node = LazyTreeNodeRole::nodeByItem(item);
	if (!node) {
		return;
	}

	if (!d->provider->remove(node->id())) {
		const auto error = d->provider->lastError();
		if (!error.isEmpty()) {
			emit errorOccurred(error);
		}
		return;
	}

	QStandardItem* parent = item->parent() ? item->parent() : d->invisibleRoot();
	parent->removeRow(item->row());
	emit nodeRemoved(node);
}

LazyTreeNodePtr LazyTreeView::selectedNode() const {
	const QModelIndex current = currentIndex();
	if (!current.isValid()) {
		return {};
	}

	QStandardItem* item = d->model->itemFromIndex(current);
	if (!item || d->isDummyNode(item) || d->isTemporaryNode(item)) {
		return {};
	}

	return LazyTreeNodeRole::nodeByItem(item);
}

QVariant LazyTreeView::selectedNodeId() const {
	const auto node = selectedNode();
	return node ? node->id() : QVariant();
}

void LazyTreeView::startEditSelectedNode() {
	startEdit(currentIndex());
}

void LazyTreeView::startEdit(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item || d->isDummyNode(item)) {
		return;
	}

	if (d->editedIndex.isValid() && d->editedIndex == QPersistentModelIndex(index)) {
		return;
	}

	d->finishCurrentEditing();

	Qt::ItemFlags flags = item->flags();
	flags &= ~Qt::ItemIsDragEnabled;
	flags &= ~Qt::ItemIsDropEnabled;
	flags |= Qt::ItemIsEditable;
	item->setFlags(flags);

	d->editedIndex = QPersistentModelIndex(index);
	setCurrentIndex(index);
	edit(index);

	if (state() != EditingState) {
		// Редактор не создан: возвращаем элемент в обычное состояние.
		d->editedIndex = QPersistentModelIndex();
		d->resetItemFlags(item);
	}
}

bool LazyTreeView::addNodeAndStartEdit(QStandardItem* parentItem, LazyTreeNodePtr newNode) {
	if (!newNode) {
		return false;
	}

	if (!parentItem) {
		parentItem = d->invisibleRoot();
	}

	if (d->isDummyNode(parentItem) || d->isTemporaryNode(parentItem)) {
		qWarning() << "LazyTreeView::addNodeAndStartEdit: parent is not editable";
		return false;
	}

	d->finishCurrentEditing();

	// Ленивый узел: сначала загружаем существующих детей, иначе они потеряются.
	if (const auto state = LazyTreeNodeRole::nodeStateByItem(parentItem); state && state->needFetchChildren()) {
		d->requestChildren(parentItem);
		if (state->needFetchChildren()) {
			return false;
		}
	}

	const auto parentNode = LazyTreeNodeRole::nodeByItem(parentItem);
	if (!parentNode) {
		return false;
	}

	newNode->setParentId(parentNode->id());

	auto* item = new QStandardItem(QString());
	auto nodeState = std::make_shared<LazyTreeNodeState>();
	nodeState->setTemporary(true);
	LazyTreeNodeRole::updateNodeState(item, nodeState);
	LazyTreeNodeRole::updateNodeRaw(item, newNode);
	item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable);

	parentItem->appendRow(item);

	const QModelIndex parentIndex = d->model->indexFromItem(parentItem);
	if (parentIndex.isValid()) {
		setExpanded(parentIndex, true);
	}

	const QModelIndex index = d->model->indexFromItem(item);
	d->editedIndex = QPersistentModelIndex(index);
	setCurrentIndex(index);
	edit(index);

	if (state() != EditingState) {
		// Редактор не создан: новый узел без имени сохранять нечего.
		d->editedIndex = QPersistentModelIndex();
		d->removeTemporaryNode(item);
		return false;
	}

	return true;
}

void LazyTreeView::finalizeEditItem(QStandardItem* item) {
	d->finalizeNodeEdit(item);
}

void LazyTreeView::expandAndSelectNode(const QVariant& id) {
	QStandardItem* item = findNode(id);
	if (!item) {
		return;
	}

	for (QStandardItem* parent = item->parent(); parent; parent = parent->parent()) {
		const QModelIndex parentIndex = d->model->indexFromItem(parent);
		if (parentIndex.isValid() && parent != d->invisibleRoot()) {
			setExpanded(parentIndex, true);
		}
	}

	const QModelIndex index = d->model->indexFromItem(item);
	if (!index.isValid()) {
		return;
	}

	setExpanded(index, true);
	scrollTo(index, QAbstractItemView::PositionAtCenter);
	setCurrentIndex(index);
	setFocus();
}

bool LazyTreeView::moveNode(const QVariant& nodeId, const QVariant& newParentId, int row) {
	if (!d->provider) {
		return false;
	}

	QStandardItem* item = findNode(nodeId);
	if (!item || d->isDummyNode(item) || d->isTemporaryNode(item)) {
		return false;
	}

	const auto node = LazyTreeNodeRole::nodeByItem(item);
	if (!node) {
		return false;
	}

	QStandardItem* newParentItem = d->parentItemForId(newParentId);
	if (!newParentItem || d->isDummyNode(newParentItem) || d->isTemporaryNode(newParentItem)) {
		return false;
	}

	if (const auto state = LazyTreeNodeRole::nodeStateByItem(newParentItem);
		state && state->needFetchChildren()) {
		// Новый родитель ещё не загружен — порядок детей не определён.
		return false;
	}

	// Запрещаем перемещение узла внутрь собственного поддерева.
	for (QStandardItem* current = newParentItem; current; current = current->parent()) {
		if (current == item) {
			return false;
		}
	}

	const auto newParentNode = LazyTreeNodeRole::nodeByItem(newParentItem);
	if (!newParentNode) {
		return false;
	}

	QStandardItem* oldParentItem = item->parent() ? item->parent() : d->invisibleRoot();
	const int oldRow = item->row();
	const QVariant previousParentId = node->parentId();

	node->setParentId(newParentNode->id());
	if (!d->provider->update(node)) {
		node->setParentId(previousParentId);
		const auto error = d->provider->lastError();
		if (!error.isEmpty()) {
			emit errorOccurred(error);
		}
		return false;
	}

	int targetRow = row < 0 ? newParentItem->rowCount() : row;
	if (oldParentItem == newParentItem && targetRow > oldRow) {
		targetRow--;
	}

	QList<QStandardItem*> rowItems = oldParentItem == d->invisibleRoot()
		? d->model->takeRow(oldRow)
		: oldParentItem->takeRow(oldRow);

	if (rowItems.isEmpty()) {
		// Модель не отдала строку: возвращаем данные в согласованное состояние.
		node->setParentId(previousParentId);
		d->provider->update(node);
		return false;
	}

	targetRow = qBound(0, targetRow, newParentItem->rowCount());
	if (newParentItem == d->invisibleRoot()) {
		d->model->insertRow(targetRow, rowItems);
	}
	else {
		newParentItem->insertRow(targetRow, rowItems);
	}

	const QModelIndex newParentIndex = d->model->indexFromItem(newParentItem);
	if (newParentIndex.isValid()) {
		setExpanded(newParentIndex, true);
	}

	emit nodeMoved(node, newParentNode);
	return true;
}

void LazyTreeView::onExpanded(const QModelIndex& index) {
	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item) {
		return;
	}

	const auto state = LazyTreeNodeRole::nodeStateByItem(item);
	if (!state || state->dummy() || !state->needFetchChildren()) {
		// Дети уже переданы или узел является листом — запрос не нужен.
		return;
	}

	d->requestChildren(item);
}

void LazyTreeView::onSelectionChanged(const QModelIndex& current, const QModelIndex& previous) {
	Q_UNUSED(previous);

	if (!current.isValid()) {
		emit currentNodeChanged(LazyTreeNodePtr());
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(current);
	if (!item || d->isDummyNode(item) || d->isTemporaryNode(item)) {
		emit currentNodeChanged(LazyTreeNodePtr());
		return;
	}

	emit currentNodeChanged(LazyTreeNodeRole::nodeByItem(item));
}

void LazyTreeView::onNodeActivated(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item || d->isDummyNode(item) || d->isTemporaryNode(item)) {
		return;
	}

	const auto node = LazyTreeNodeRole::nodeByItem(item);
	if (node) {
		emit nodeActivated(node);
	}
}

void LazyTreeView::Private::buildSubTree(QStandardItem* parentItem, const LazyTreeNodeList& nodes) {
	for (const auto& nodeData : nodes) {
		if (!nodeData) {
			continue;
		}

		auto* item = new QStandardItem(nodeData->name());
		auto nodeState = std::make_shared<LazyTreeNodeState>();
		LazyTreeNodeRole::updateNodeState(item, nodeState);
		LazyTreeNodeRole::updateNodeRaw(item, nodeData);
		item->setFlags(
			Qt::ItemIsEnabled | Qt::ItemIsSelectable |
			Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);

		parentItem->appendRow(item);

		const auto children = nodeData->children();
		if (!children.has_value()) {
			continue;
		}

		if (children->empty()) {
			// Дети есть, но их нужно загрузить при раскрытии.
			nodeState->setNeedFetchChildren(true);
			addDummyNode(item);
			continue;
		}

		buildSubTree(item, *children);
		if (nodeData->expanded()) {
			q->setExpanded(model->indexFromItem(item), true);
		}
	}
}

void LazyTreeView::Private::addDummyNode(QStandardItem* parentItem) {
	if (!parentItem) {
		return;
	}

	auto dummy = new QStandardItem(LazyTreeView::tr("Загрузка…"));
	auto nodeState = std::make_shared<LazyTreeNodeState>();
	nodeState->setDummy(true);
	LazyTreeNodeRole::updateNodeState(dummy, nodeState);
	dummy->setFlags(Qt::NoItemFlags);
	parentItem->appendRow(dummy);

	if (const auto parentState = LazyTreeNodeRole::nodeStateByItem(parentItem)) {
		parentState->setNeedFetchChildren(true);
	}
}

bool LazyTreeView::Private::isDummyNode(QStandardItem* item) const {
	if (!item) {
		return false;
	}

	const auto state = LazyTreeNodeRole::nodeStateByItem(item);
	return state && state->dummy();
}

bool LazyTreeView::Private::isTemporaryNode(QStandardItem* item) const {
	if (!item) {
		return false;
	}

	const auto state = LazyTreeNodeRole::nodeStateByItem(item);
	return state && state->temporary();
}

void LazyTreeView::Private::prepareRootItem() {
	auto* rootItem = model->invisibleRootItem();
	auto nodeState = std::make_shared<LazyTreeNodeState>();
	LazyTreeNodeRole::updateNodeState(rootItem, nodeState);
	LazyTreeNodeRole::updateNodeRaw(rootItem, rootNode);
}

void LazyTreeView::Private::removeDummyNodes(QStandardItem* parentItem) {
	if (!parentItem) {
		return;
	}

	for (int i = parentItem->rowCount() - 1; i >= 0; i--) {
		if (isDummyNode(parentItem->child(i))) {
			parentItem->removeRow(i);
		}
	}
}

void LazyTreeView::Private::removeTemporaryNode(QStandardItem* item) {
	if (!item || !isTemporaryNode(item)) {
		return;
	}

	if (editedIndex.isValid() && model->itemFromIndex(editedIndex) == item) {
		editedIndex = QPersistentModelIndex();
	}

	QStandardItem* parent = item->parent() ? item->parent() : invisibleRoot();
	parent->removeRow(item->row());
}

void LazyTreeView::Private::finalizeNodeEdit(QStandardItem* item) {
	if (!item || finalizing) {
		return;
	}

	FinalizeGuard guard(finalizing);

	const auto nodeState = LazyTreeNodeRole::nodeStateByItem(item);
	const auto node = LazyTreeNodeRole::nodeByItem(item);
	if (!nodeState || !node) {
		return;
	}

	const QString currentText = item->text().trimmed();
	const bool temporary = nodeState->temporary();

	if (temporary) {
		if (currentText.isEmpty()) {
			// Пустое имя: ветка не сохраняется.
			removeTemporaryNode(item);
			return;
		}

		if (!isValidSiblingName(item, currentText)) {
			removeTemporaryNode(item);
			emit q->errorOccurred(LazyTreeView::tr("Узел с именем «%1» уже существует").arg(currentText));
			return;
		}

		node->setName(currentText);

		if (provider && provider->add(node)) {
			item->setText(currentText);
			nodeState->setTemporary(false);
			resetItemFlags(item);
			emit q->nodeAdded(node);
		}
		else {
			const QString error = provider ? provider->lastError() : QString();
			removeTemporaryNode(item);
			if (!error.isEmpty()) {
				emit q->errorOccurred(error);
			}
		}

		return;
	}

	// Существующий узел: переименование.
	if (currentText.isEmpty()) {
		item->setText(node->name());
		resetItemFlags(item);
		return;
	}

	if (currentText == node->name()) {
		resetItemFlags(item);
		return;
	}

	if (!isValidSiblingName(item, currentText)) {
		item->setText(node->name());
		resetItemFlags(item);
		emit q->errorOccurred(LazyTreeView::tr("Узел с именем «%1» уже существует").arg(currentText));
		return;
	}

	const QString previousName = node->name();
	node->setName(currentText);

	if (provider && provider->update(node)) {
		item->setText(currentText);
		emit q->nodeRenamed(node, currentText);
	}
	else {
		node->setName(previousName);
		item->setText(previousName);
		const QString error = provider ? provider->lastError() : QString();
		if (!error.isEmpty()) {
			emit q->errorOccurred(error);
		}
	}

	resetItemFlags(item);
}

void LazyTreeView::Private::finishCurrentEditing() {
	if (!editedIndex.isValid()) {
		return;
	}

	const QPersistentModelIndex index = editedIndex;
	editedIndex = QPersistentModelIndex();

	if (auto* item = model->itemFromIndex(index)) {
		finalizeNodeEdit(item);
	}
}

bool LazyTreeView::Private::isValidSiblingName(QStandardItem* item, const QString& name) const {
	if (!item) {
		return false;
	}

	QStandardItem* parent = item->parent() ? item->parent() : invisibleRoot();
	for (int i = 0; i < parent->rowCount(); i++) {
		QStandardItem* sibling = parent->child(i);
		if (!sibling || sibling == item || isDummyNode(sibling) || isTemporaryNode(sibling)) {
			continue;
		}

		if (sibling->text().trimmed().compare(name, Qt::CaseInsensitive) == 0) {
			return false;
		}
	}

	return true;
}

QStandardItem* LazyTreeView::Private::parentItemForId(const QVariant& parentId) const {
	if (!parentId.isValid() || parentId.isNull()) {
		return invisibleRoot();
	}

	return q->findNode(parentId);
}

QStandardItem* LazyTreeView::Private::invisibleRoot() const {
	return model->invisibleRootItem();
}

void LazyTreeView::Private::resetItemFlags(QStandardItem* item) {
	if (!item) {
		return;
	}

	Qt::ItemFlags flags = item->flags();
	flags &= ~Qt::ItemIsEditable;
	flags |= Qt::ItemIsEnabled | Qt::ItemIsSelectable |
		Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
	item->setFlags(flags);
}

void LazyTreeView::Private::requestChildren(QStandardItem* item) {
	if (!provider || !item) {
		return;
	}

	const auto nodeState = LazyTreeNodeRole::nodeStateByItem(item);
	const auto node = LazyTreeNodeRole::nodeByItem(item);
	if (!nodeState || !node) {
		return;
	}

	const QVariant parentId = node->id();

	// Заглушка не должна остаться в списке детей.
	removeDummyNodes(item);

	const LazyTreeNodeList nodes = provider->get(parentId);
	if (nodes.empty()) {
		const QString error = provider->lastError();
		if (!error.isEmpty()) {
			// Оставляем заглушку: загрузку можно повторить.
			addDummyNode(item);
			emit q->errorOccurred(error);
			return;
		}

		nodeState->setNeedFetchChildren(false);
		return;
	}

	nodeState->setNeedFetchChildren(false);
	q->appendChildren(parentId, nodes);
}

void LazyTreeView::Private::removeAllChildren(QStandardItem* parentItem) {
	if (!parentItem) {
		return;
	}

	const int count = parentItem->rowCount();
	if (count > 0) {
		parentItem->removeRows(0, count);
	}
}
