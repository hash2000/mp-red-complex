#pragma once
#include "Content/InventroiesModule/data_providers/i_inventories_data_provider.h"

class DatabasesService;

class InventoriesDataProviderDb : public IInventoriesDataProvider {
public:
	InventoriesDataProviderDb(DatabasesService* databasesService);
	~InventoriesDataProviderDb() override;

	void loadInventories(std::list<QUuid>& inventories) const override;
	void loadEquipments(std::list<QUuid>& equipments) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
