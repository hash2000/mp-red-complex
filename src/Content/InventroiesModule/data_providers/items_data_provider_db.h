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

	std::shared_ptr<Item> item(const QUuid& id) const override;
	std::list<std::shared_ptr<Item>> containers() const override;
	std::list<std::shared_ptr<Item>> equipments() const override;
	std::list<std::shared_ptr<Item>> itemsFromContainer(const QUuid& constainerId) const override;
	bool changeContainer(const Item& item, const QUuid& constainerId) const;
	bool save(const Item& item) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
