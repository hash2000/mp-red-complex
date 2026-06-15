#pragma once
#include "Content/InventroiesModule/data_providers/i_items_data_provider.h"
#include <QPixmap>
#include <QUuid>
#include <memory>

class DatabasesService;

class ItemsDataProviderDb : public IItemsDataProvider {
public:
	ItemsDataProviderDb(DatabasesService* databasesService);
	~ItemsDataProviderDb() override;

	std::list<std::unique_ptr<ItemEntity>> entities() const override;
	std::unique_ptr<ItemEntity> entity(const QString& id) const override;
	std::unique_ptr<Item> item(const QUuid& id) const override;
	bool setItem(const QUuid& id, const Item& item) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
