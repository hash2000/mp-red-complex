#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include <QString>

#include <optional>
#include <vector>

class FetchTree : public ILazyTreeNode {
public:
	~FetchTree() override = default;

	void setId(int i) override { _id = i; }
	int id() const override { return _id; }
	void setParentId(std::optional<int> pId) override { _parentId = pId; }
	std::optional<int> parentId() const override { return _parentId; }
	void setName(const QString& n) override { _name = n; }
	QString name() const override { return _name; }
	std::optional<std::vector<std::shared_ptr<ILazyTreeNode>>> children() const override { return _children; }
	void setChildren(const std::vector<std::shared_ptr<ILazyTreeNode>>& ch) { _children = ch; }
	void clearChildren() { _children = std::nullopt; }
	void setExpanded(bool value) override { _expanded = value; }
	bool expanded() const { return _expanded; }

private:
	int _id;
	std::optional<int> _parentId;
	QString _name;
	bool _expanded;

	std::optional<std::vector<std::shared_ptr<ILazyTreeNode>>> _children;
};
