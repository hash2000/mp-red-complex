#pragma once
#include "DataLayer/inventory/inventory_data_provider.h"
#include <QString>
#include <memory>

class Resources;

class InventoryDataProviderFilesistemImpl : public InventoryDataProvider {
public:
	InventoryDataProviderFilesistemImpl(Resources* resources);
	~InventoryDataProviderFilesistemImpl() override;

	std::optional<Inventory> fromContainer(const QUuid& id) const override;
	std::optional<InventoryItem> getItem(const QString& id) const override;
	QPixmap loadIcon(const InventoryItem& item) const override;

private:
	static QPixmap loadEmptyStubIcon(const QString& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
