#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/data_providers/lazy_tree_data_provider_db.h"
#include <QObject>
#include <memory>

class DatabasesService;

class FetchStoreDbService : public QObject, public LazyNodesDataProviderDb {
public:
	FetchStoreDbService(DatabasesService* databaseService, QObject* parent = nullptr);
	~FetchStoreDbService() override;

	LazyTreeNodePtr createTreeNode() override;
	LazyTreeNodePtr createRootNode() override;

protected:
	void onProcessNodeAfterInsert(const LazyTreeNodePtr& node, long lastInsertId) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
