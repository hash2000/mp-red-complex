#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"

#include <memory>

/// Универсальная реализация ILazyTreeNode.
///
/// Используется поставщиками данных, которым не нужны собственные поля:
/// идентификатор, имя, родитель, флаг раскрытия, дети и JSON-свойства.
class LazyTreeDataNode : public ILazyTreeNode {
public:
	LazyTreeDataNode();
	explicit LazyTreeDataNode(QVariant id, QVariant parentId = {}, QString name = {});
	~LazyTreeDataNode() override;

	void setId(QVariant id) override;
	QVariant id() const override;
	void setParentId(QVariant parentId) override;
	QVariant parentId() const override;
	void setName(const QString& name) override;
	QString name() const override;
	void setExpanded(bool value) override;
	bool expanded() const override;

	void appendChild(const LazyTreeNodePtr& ch) override;
	void setChildren(const LazyTreeNodeList& ch) override;
	void clearChildren() override;
	std::optional<LazyTreeNodeList> children() const override;

	QVariantMap properties() const override;
	bool setProperty(const QString& name, const QVariant& value) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
