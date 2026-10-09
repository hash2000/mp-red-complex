#pragma once
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_data_provider.h"
#include <QString>

class DatabasesService;

class LazyNodesDataProviderDb : public ILazyNodesDataProvider {
public:
	LazyNodesDataProviderDb(DatabasesService* databasesService, const QString& databaseName, const QString& tableName);
	~LazyNodesDataProviderDb() override;

	LazyTreeNodeList get(const QVariant& parentId) const override;
	LazyTreeNodeList search(const QString& text) const override;
	bool add(const LazyTreeNodePtr& node) override;
	bool remove(const QVariant& id) override;
	bool update(const LazyTreeNodePtr& node) override;

protected:
	virtual void onProcessNodeAfterInsert(const LazyTreeNodePtr& node, long lastInsertId) { };

private:
	class Private;
	std::unique_ptr<Private> d;
};
