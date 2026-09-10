#include "Content/FetchApiModule/widgets/fetch_queries_tree_widget.h"
#include "Content/FetchApiModule/action_handlers/fetch_queries_tree_action_handler.h"

class FetchQueriesTreeWidget::Private {
public:
	Private(FetchQueriesTreeWidget* parent, ILazyNodesDataProvider* provider)
		: q(parent) {
		actionHandler = std::make_unique<LazyTreeNodeActionHandler>(provider, parent);
	}

	FetchQueriesTreeWidget* q;
	std::unique_ptr<ILazyTreeNodeActionHandler> actionHandler;
};

FetchQueriesTreeWidget::FetchQueriesTreeWidget(ILazyNodesDataProvider* provider, QWidget* parent)
	: d(std::make_unique<Private>(this, provider))
	, LazyTreeWidget(provider, parent) {
	setActionHandler(d->actionHandler.get());

	connect(this, &LazyTreeWidget::nodeEditRequested, this, &FetchQueriesTreeWidget::onNodeActivated);
}

FetchQueriesTreeWidget::~FetchQueriesTreeWidget() = default;

void FetchQueriesTreeWidget::onNodeActivated(const LazyTreeNodePtr& node) {

}
