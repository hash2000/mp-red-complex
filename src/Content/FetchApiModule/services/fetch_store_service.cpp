#include "Content/FetchApiModule/services/fetch_store_service.h"

class FetchStoreService::Private {
public:
	Private(FetchStoreService* parent) : q(parent) {}
	FetchStoreService* q;

	IFetchQueriesDataProvider* queriesDataProvider;
	IFetchTreeNodesDataProvider* treeNodesDataProvider;
};

FetchStoreService::FetchStoreService(
	IFetchQueriesDataProvider* queriesDataProvider,
	IFetchTreeNodesDataProvider* treeNodesDataProvider
	, QObject* parent)
	: d(std::make_unique<Private>(this))
	, QObject(parent) {
	d->queriesDataProvider = queriesDataProvider;
	d->treeNodesDataProvider = treeNodesDataProvider;
}

FetchStoreService::~FetchStoreService() = default;
