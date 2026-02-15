#pragma once
#include "DataLayer/inventory/inventory_data_provider.h"
#include <QString>
#include <memory>

class Resources;

class InventoryDataProviderFilesistemImpl : public InventoryDataProvider {
public:
	InventoryDataProviderFilesistemImpl(Resources* resources);
	~InventoryDataProviderFilesistemImpl() override;

	std::shared_ptr<Inventory> loadInventory(const QUuid& id) const override;

private:

	bool loadItem(InventoryItem& item) const;

	QPixmap loadIcon(const InventoryItem& item) const;
	static QPixmap loadEmptyStubIcon(const QString& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
