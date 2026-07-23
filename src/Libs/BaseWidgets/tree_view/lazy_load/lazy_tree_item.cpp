#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_item.h"
#include <QList>

class LazyTreeItem::Private {
public:
	Private(LazyTreeItem* parent) : q(parent) {}
	LazyTreeItem* q;


	std::shared_ptr<ILazyTreeNode> data;
	LazyTreeItem* parent;
	QList<LazyTreeItem*> childItems;
	bool childrenFetched = false;
	bool isTemporary = false;
};

LazyTreeItem::LazyTreeItem(LazyTreeNodePtr data, LazyTreeItem* parent)
	: d(std::make_unique<Private>(this)) {
	d->data = data;
	d->parent = parent;
}

LazyTreeItem::~LazyTreeItem() {
	qDeleteAll(d->childItems);
}

void LazyTreeItem::appendChild(LazyTreeItem* child) {
	d->childItems.append(child);
}

LazyTreeItem* LazyTreeItem::child(int row) {
	return d->childItems.value(row);
}

int LazyTreeItem::childCount() const {
	return d->childItems.count();
}

int LazyTreeItem::row() const {
	if (d->parent) {
		return d->parent->d->childItems.indexOf(const_cast<LazyTreeItem*>(this));
	}

	return 0;
}

LazyTreeItem* LazyTreeItem::parent() {
	return d->parent;
}

LazyTreeNodePtr LazyTreeItem::data() const {
	return d->data;
}

bool LazyTreeItem::isChildrenFetched() const {
	return d->childrenFetched;
}

void LazyTreeItem::setChildrenFetched(bool fetched) {
	d->childrenFetched = fetched;
}

bool LazyTreeItem::isTemporary() const {
	return d->isTemporary;
}

void LazyTreeItem::setIsTemporary(bool temp) {
	d->isTemporary = temp;
}
