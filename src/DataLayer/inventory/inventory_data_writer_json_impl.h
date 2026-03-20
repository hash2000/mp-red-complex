#pragma once
#include "DataLayer/inventory/i_inventory_data_writer.h"
#include <QString>
#include <memory>

class Resources;

class InventoryDataWriterJsonImpl : public IInventoryDataWriter {
public:
	InventoryDataWriterJsonImpl(Resources* resources);
	~InventoryDataWriterJsonImpl() override;

	bool saveInventory(const QUuid& id, const Inventory& inventory) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
