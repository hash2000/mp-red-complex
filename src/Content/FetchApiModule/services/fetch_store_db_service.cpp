#include "Content/FetchApiModule/services/fetch_store_db_service.h"
#include "Content/FetchApiModule/models/fetch_tree.h"

class FetchStoreDbService::Private {
public:
	Private(FetchStoreDbService* parent) : q(parent) { }
	FetchStoreDbService* q;
};

FetchStoreDbService::FetchStoreDbService(DatabasesService* databaseService, QObject* parent)
	: d(std::make_unique<Private>(this))
	, LazyNodesDataProviderDb(databaseService, "fetch_api", "queries")
	, QObject(parent) {
}

FetchStoreDbService::~FetchStoreDbService() = default;

LazyTreeNodePtr FetchStoreDbService::createTreeNode() {
	return std::make_shared<FetchTree>();
}

LazyTreeNodePtr FetchStoreDbService::createRootNode() {
	return std::make_shared<FetchTree>();
}

void FetchStoreDbService::onProcessNodeAfterInsert(const LazyTreeNodePtr& node, long lastInsertId) {
	node->setId(QVariant::fromValue((int)lastInsertId));
}

