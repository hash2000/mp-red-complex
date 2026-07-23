#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"

class LazyTreeItem {
public:
	explicit LazyTreeItem(LazyTreeNodePtr data, LazyTreeItem* parent = nullptr);
	~LazyTreeItem();

	void appendChild(LazyTreeItem* child);
	LazyTreeItem* child(int row);
	int childCount() const;
	int row() const;
	LazyTreeItem* parent();
	LazyTreeNodePtr data() const;

	bool isChildrenFetched() const;
	void setChildrenFetched(bool fetched);

	bool isTemporary() const;
	void setIsTemporary(bool temp);

private:
	class Private;
	std::unique_ptr<Private> d;
};
