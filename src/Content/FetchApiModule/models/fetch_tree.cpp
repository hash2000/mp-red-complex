#include "Content/FetchApiModule/models/fetch_tree.h"
#include <optional>

class FetchTree::Private {
public:
	Private(FetchTree* parent) : q(parent) {}
	FetchTree* q;

	std::optional<int> id;
	QVariant parentId;
	QString name;
	QString path;
	bool expanded = false;
	bool needChildren = false;

	std::optional<LazyTreeNodeList> children;
};


FetchTree::FetchTree()
	: d(std::make_unique<Private>(this)) {

}

FetchTree::~FetchTree() = default;

void FetchTree::setId(QVariant i) {
	if (i.isValid()) {
		d->id = i.toInt();
	}
	else {
		d->id = std::nullopt;
	}
}

QVariant FetchTree::id() const {
	if (d->id.has_value()) {
		return QVariant::fromValue(d->id.value());
	}
	return QVariant();
}

void FetchTree::setParentId(QVariant pId) {
	d->parentId = pId;
}

QVariant FetchTree::parentId() const {
	return d->parentId;
}

void FetchTree::setName(const QString& n) {
	d->name = n;
}

QString FetchTree::name() const {
	return d->name;
}

void FetchTree::appendChild(const LazyTreeNodePtr& ch) {
	if (!d->children.has_value()) {
		d->children = LazyTreeNodeList();
	}

	d->children->push_back(ch);
}

void FetchTree::setChildren(const LazyTreeNodeList& ch) {
	d->children = ch;
}

void FetchTree::clearChildren() {
	d->children = std::nullopt;
}

void FetchTree::setExpanded(bool value) {
	d->expanded = value;
}

bool FetchTree::expanded() const {
	return d->expanded;
}

std::optional<LazyTreeNodeList> FetchTree::children() const {
	return d->children;
}
