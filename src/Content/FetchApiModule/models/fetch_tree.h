#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

#include <memory>

class FetchTree : public ILazyTreeNode {
public:
	FetchTree();
	~FetchTree() override;

	void setId(QVariant i) override;
	QVariant id() const override;
	void setParentId(QVariant pId) override;
	QVariant parentId() const override;
	void setName(const QString& n) override;
	QString name() const override;
	void setChildren(const LazyTreeNodeList& ch) override;
	void clearChildren() override;
	void setExpanded(bool value) override;
	bool expanded() const override;

	void appendChild(const LazyTreeNodePtr& ch);
	std::optional<LazyTreeNodeList> children() const;

	QVariantMap properties() const override;
	bool setProperty(const QString& name, const QVariant& value) override;


private:
	class Private;
	std::unique_ptr<Private> d;
};
