#pragma once
#include <QString>
#include <QVariant>
#include <vector>
#include <memory>
#include <optional>

using LazyTreeNodePtr = std::shared_ptr<class ILazyTreeNode>;
using LazyTreeNodeList = std::vector<LazyTreeNodePtr>;

class ILazyTreeNode {
public:
	virtual ~ILazyTreeNode() = default;
	virtual void setId(QVariant id) = 0;
	virtual QVariant id() const = 0;
	virtual void setParentId(QVariant parentId) = 0;
	virtual QVariant parentId() const = 0;
	virtual void setName(const QString& name) = 0;
	virtual QString name() const = 0;
	virtual void setExpanded(bool value) = 0;
	virtual bool expanded() const = 0;

	virtual bool equals(const LazyTreeNodePtr& node) const = 0;

	virtual void appendChild(const LazyTreeNodePtr& ch) = 0;
	virtual void setChildren(const LazyTreeNodeList& ch) = 0;
	virtual void clearChildren() = 0;
	virtual std::optional<LazyTreeNodeList> children() const = 0;
};

class ILazyNodesDataProvider {
public:
	virtual ~ILazyNodesDataProvider() = default;
	virtual LazyTreeNodePtr createTreeNode() = 0;
	virtual LazyTreeNodeList treeNodes(const QVariant& parentId) const = 0;
	virtual LazyTreeNodeList searchTreeNodes(const QString& text) const = 0;
	virtual bool addTreeNode(const LazyTreeNodePtr& node) = 0;
	virtual bool deleteTreeNode(const QVariant& id) = 0;
	virtual bool updateTreeNode(const LazyTreeNodePtr& node) = 0;
};
