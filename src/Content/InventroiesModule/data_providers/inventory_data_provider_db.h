#pragma once
#include "Content/InventroiesModule/data_providers/i_inventory_data_provider.h"
#include <QString>
#include <memory>

class DatabasesService;

class InventoryDataProviderDb : public IInventoryDataProvider {
public:
	InventoryDataProviderDb(DatabasesService* databasesService);
	~InventoryDataProviderDb() override;

	std::shared_ptr<Inventory> loadInventory(const QUuid& id) const override;
	bool saveInventory(const QUuid& id, const Inventory& inventory) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
