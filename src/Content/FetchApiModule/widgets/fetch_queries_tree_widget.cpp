#include "Content/FetchApiModule/widgets/fetch_queries_tree_widget.h"

class FetchQueriesTreeWidget::Private {
public:
	Private(FetchQueriesTreeWidget* parent)
		: q(parent) {}

	FetchQueriesTreeWidget* q;

};

FetchQueriesTreeWidget::FetchQueriesTreeWidget(ILazyNodesDataProvider* provider, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, LazyTreeViewWidget(provider, parent) {

}

FetchQueriesTreeWidget::~FetchQueriesTreeWidget() = default;
