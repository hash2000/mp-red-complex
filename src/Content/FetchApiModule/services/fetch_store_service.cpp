#include "Content/FetchApiModule/services/fetch_store_service.h"
#include "Content/FetchApiModule/data_providers/i_fetch_queries_data_provider.h"
#include "Content/FetchApiModule/data_providers/i_fetch_tree_nodes_data_provider.h"
#include "Content/FetchApiModule/models/fetch_opt.h"
#include "Content/FetchApiModule/models/fetch_query.h"
#include "Content/FetchApiModule/models/fetch_tree.h"

#include <optional>

class FetchStoreService::Private {
public:
	Private(
		IFetchQueriesDataProvider* q,
		IFetchTreeNodesDataProvider* t,
		FetchStoreService* parent)
		: q(parent)
		, queriesDataProvider(q)
		, treeNodesDataProvider(t) {
	}

	FetchStoreService* q;

	IFetchQueriesDataProvider* queriesDataProvider;
	IFetchTreeNodesDataProvider* treeNodesDataProvider;
};

FetchStoreService::FetchStoreService(
	IFetchQueriesDataProvider* queriesDataProvider,
	IFetchTreeNodesDataProvider* treeNodesDataProvider
	, QObject* parent)
	: d(std::make_unique<Private>(queriesDataProvider, treeNodesDataProvider, this))
	, QObject(parent) {
}

FetchStoreService::~FetchStoreService() = default;

LazyTreeNodePtr FetchStoreService::createTreeNode() {
	return std::make_shared<FetchTree>();
}

LazyTreeNodeList FetchStoreService::treeNodes(const LazyTreeNodePtr& parentNode) const {
	return d->treeNodesDataProvider->treeNodes(parentNode ? std::optional<int>(parentNode->id()) : std::nullopt);
}

LazyTreeNodeList FetchStoreService::searchTreeNodes(const QString& text) const {
	return d->treeNodesDataProvider->searchTreeNodes(text);
}

bool FetchStoreService::addTreeNode(const LazyTreeNodePtr& node) {
	return d->treeNodesDataProvider->addNode(node);
}

bool FetchStoreService::deleteTreeNode(const LazyTreeNodePtr& node) {
	return d->treeNodesDataProvider->deleteTreeNode(node->id());
}

bool FetchStoreService::updateTreeNode(const LazyTreeNodePtr& node) {
	return d->treeNodesDataProvider->updateTreeNode(node);
}

