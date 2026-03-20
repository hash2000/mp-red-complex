#pragma once
#include "DataLayer/inventories/i_inventories_data_provider.h"

class Resources;

class InventoriesDataProviderJsonImpl : public IInventoriesDataProvider {
public:
	InventoriesDataProviderJsonImpl(Resources* resources);
	~InventoriesDataProviderJsonImpl() override;

	void loadInventories(std::list<QUuid>& inventories) const override;

	void loadEquipments(std::list<QUuid>& equipments) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
