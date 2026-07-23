#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_widget.h"

#include <QWidget>
#include <memory>

class ILazyNodesDataProvider;

class FetchQueriesTreeWidget : public LazyTreeViewWidget {
	Q_OBJECT
public:
	explicit FetchQueriesTreeWidget(ILazyNodesDataProvider* provider, QWidget* parent = nullptr);
	~FetchQueriesTreeWidget() override;


private:
	class Private;
	std::unique_ptr<Private> d;
};
