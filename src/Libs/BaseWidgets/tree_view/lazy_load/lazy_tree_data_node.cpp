#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"

#include <QVariantMap>

class LazyTreeDataNode::Private {
public:
	QVariant id;
	QVariant parentId;
	QString name;
	bool expanded = false;
	QVariantMap properties;
	std::optional<LazyTreeNodeList> children;
};

LazyTreeDataNode::LazyTreeDataNode()
	: d(std::make_unique<Private>()) {
}

LazyTreeDataNode::LazyTreeDataNode(QVariant id, QVariant parentId, QString name)
	: d(std::make_unique<Private>()) {
	d->id = std::move(id);
	d->parentId = std::move(parentId);
	d->name = std::move(name);
}

LazyTreeDataNode::~LazyTreeDataNode() = default;

void LazyTreeDataNode::setId(QVariant id) {
	d->id = std::move(id);
}

QVariant LazyTreeDataNode::id() const {
	return d->id;
}

void LazyTreeDataNode::setParentId(QVariant parentId) {
	d->parentId = std::move(parentId);
}

QVariant LazyTreeDataNode::parentId() const {
	return d->parentId;
}

void LazyTreeDataNode::setName(const QString& name) {
	d->name = name;
}

QString LazyTreeDataNode::name() const {
	return d->name;
}

void LazyTreeDataNode::setExpanded(bool value) {
	d->expanded = value;
}

bool LazyTreeDataNode::expanded() const {
	return d->expanded;
}

void LazyTreeDataNode::appendChild(const LazyTreeNodePtr& ch) {
	if (!ch) {
		return;
	}

	if (!d->children.has_value()) {
		d->children = LazyTreeNodeList();
	}

	d->children->push_back(ch);
}

void LazyTreeDataNode::setChildren(const LazyTreeNodeList& ch) {
	d->children = ch;
}

void LazyTreeDataNode::clearChildren() {
	d->children = std::nullopt;
}

std::optional<LazyTreeNodeList> LazyTreeDataNode::children() const {
	return d->children;
}

QVariantMap LazyTreeDataNode::properties() const {
	return d->properties;
}

bool LazyTreeDataNode::setProperty(const QString& name, const QVariant& value) {
	if (name.isEmpty()) {
		return false;
	}

	d->properties.insert(name, value);
	return true;
}
