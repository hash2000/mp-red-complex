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

	virtual void appendChild(const LazyTreeNodePtr& ch) = 0;
	virtual void setChildren(const LazyTreeNodeList& ch) = 0;
	virtual void clearChildren() = 0;
	virtual std::optional<LazyTreeNodeList> children() const = 0;

	virtual QVariantMap properties() const = 0;
	virtual bool setProperty(const QString& name, const QVariant& value) = 0;
};
