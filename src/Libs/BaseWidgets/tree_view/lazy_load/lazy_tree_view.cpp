#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_view.h"

#include <QMouseEvent>
#include <QStandardItemModel>
#include <QVariant>

namespace {
constexpr int RoleNodeRawData = Qt::UserRole + 1;
constexpr int RoleNodeId = Qt::UserRole + 2;
constexpr int RoleNodeIsDummy = Qt::UserRole + 3;
}

class LazyTreeView::Private {
public:
	Private(LazyTreeView* parent) : q(parent) {}
	LazyTreeView* q;

	QStandardItemModel* model;

	void buildSubTree(QStandardItem* parentItem, const LazyTreeNodeList& nodes);
	void addDummyNode(QStandardItem* parentItem);
	bool isDummyNode(QStandardItem* item) const;
	void preareRootItemModel(QStandardItemModel* model);
};

LazyTreeView::LazyTreeView(QWidget* parent)
	: d(std::make_unique<Private>(this)) {
	d->model = new QStandardItemModel(this);
	setModel(d->model);
	setHeaderHidden(true);
	setUniformRowHeights(true); // Оптимизация для больших деревьев

	connect(this, &QTreeView::expanded, this, &LazyTreeView::onExpanded);
	connect(this, &QTreeView::activated, this, &LazyTreeView::onNodeActivated);
	connect(selectionModel(), &QItemSelectionModel::currentChanged, this, &LazyTreeView::onSelectionChanged);
}

LazyTreeView::~LazyTreeView() = default;

void LazyTreeView::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		QModelIndex index = indexAt(event->pos());
		if (!index.isValid()) {
			clearSelection();
			setCurrentIndex(QModelIndex());
			emit emptyAreaClicked();
			emit selectionChanged(LazyTreeNodePtr());
			return;
		}
	}

	QTreeView::mousePressEvent(event);
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
		item->setData(nodeData->id(), RoleNodeId);
		item->setData(QVariant::fromValue(nodeData), RoleNodeRawData);
		item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable); // Не редактируется по умолчанию

		parentItem->appendRow(item);

		if (children.has_value()) {
			if (!children->empty()) {
				buildSubTree(item, *children);
			}
			else {
				addDummyNode(item);
			}
		}
	}
}

void LazyTreeView::Private::addDummyNode(QStandardItem* parentItem) {
	auto* dummy = new QStandardItem("Loading..."); // Можно оставить пустым ""
	dummy->setData(true, RoleNodeIsDummy);
	dummy->setFlags(Qt::NoItemFlags); // Неактивный, не кликабельный
	parentItem->appendRow(dummy);
}

bool LazyTreeView::Private::isDummyNode(QStandardItem* item) const {
	return item && item->data(RoleNodeIsDummy).toBool();
}

void LazyTreeView::onExpanded(const QModelIndex& index) {
	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item) {
		return;
	}

	// Если это первый раз и там dummy-узел
	if (item->rowCount() == 1 && d->isDummyNode(item->child(0))) {
		QVariant parentId = item->data(RoleNodeId);
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

		if (current->data(RoleNodeId) == id) {
			return current;
		}

		for (int i = 0; i < current->rowCount(); i++) {
			stack.append(current->child(i));
		}
	}

	return nullptr;
}

LazyTreeNodePtr LazyTreeView::nodeFromItem(const QStandardItem* item) const {
	if (!item) {
		return LazyTreeNodePtr();
	}

	return item->data(RoleNodeRawData).value<LazyTreeNodePtr>();
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

void LazyTreeView::onNodeActivated(const QModelIndex& index) {
	if (!index.isValid()) {
		return;
	}

	QStandardItem* item = d->model->itemFromIndex(index);
	if (!item || d->isDummyNode(item)) {
		return;
	}

	auto node = item->data(RoleNodeRawData).value<LazyTreeNodePtr>();
	emit nodeActivated(node, item->text());
}

void LazyTreeView::onSelectionChanged(const QModelIndex& current, const QModelIndex& previous) {
	Q_UNUSED(previous);
	if (current.isValid()) {
		QStandardItem* item = d->model->itemFromIndex(current);
		if (item && !d->isDummyNode(item)) {
			auto node = item->data(RoleNodeRawData).value<LazyTreeNodePtr>();
			emit selectionChanged(node);
			return;
		}
	}

	// root node
	emit selectionChanged(LazyTreeNodePtr());
}

void LazyTreeView::Private::preareRootItemModel(QStandardItemModel* model) {
	auto normalRootItem = model->invisibleRootItem();
	normalRootItem->setData(QVariant(), RoleNodeId);
	normalRootItem->setData(false, RoleNodeIsDummy);
}
