#pragma once
#include <QString>
#include <vector>
#include <memory>
#include <optional>

using LazyTreeNodePtr = std::shared_ptr<class ILazyTreeNode>;
using LazyTreeNodeList = std::vector<LazyTreeNodePtr>;

class ILazyTreeNode {
public:
	virtual ~ILazyTreeNode() = default;
	virtual void setId(int id) = 0;
	virtual int id() const = 0;
	virtual void setParentId(std::optional<int> parentId) = 0;
	virtual std::optional<int> parentId() const = 0;
	virtual void setName(const QString& name) = 0;
	virtual QString name() const = 0;
	virtual void setExpanded(bool value) = 0;
	virtual bool expanded() const = 0;

	virtual std::optional<LazyTreeNodeList> children() const = 0;
};

class ILazyNodesDataProvider {
public:
	virtual ~ILazyNodesDataProvider() = default;
	virtual LazyTreeNodePtr createTreeNode() = 0;
	virtual LazyTreeNodeList treeNodes(const LazyTreeNodePtr& parentNode) const = 0;
	virtual LazyTreeNodeList searchTreeNodes(const QString& text) const = 0;
	virtual bool addTreeNode(const LazyTreeNodePtr& node) = 0;
	virtual bool deleteTreeNode(const LazyTreeNodePtr& node) = 0;
	virtual bool updateTreeNode(const LazyTreeNodePtr& node) = 0;
};
