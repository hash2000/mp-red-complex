#pragma once
#include "DataLayer/inventory/inventory_data_provider.h"
#include <QString>
#include <memory>

class Resources;

class InventoryDataProviderFilesistemImpl : public InventoryDataProvider {
public:
	InventoryDataProviderFilesistemImpl(Resources* resources);
	~InventoryDataProviderFilesistemImpl() override;

	QList<InventoryItem> fromContainer(const QUuid& id) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
