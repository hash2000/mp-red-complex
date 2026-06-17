#pragma once
#include "DataLayer/inventory/i_inventory_data_provider.h"
#include <QString>
#include <memory>

class Resources;

class InventoryDataProviderJsonImpl : public IInventoryDataProvider {
public:
	InventoryDataProviderJsonImpl(Resources* resources);
	~InventoryDataProviderJsonImpl() override;

	bool loadInventory(const QUuid& id, Inventory& inventory) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
