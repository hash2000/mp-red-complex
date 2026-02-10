#pragma once
#include "DataLayer/inventory/inventory_data_provider.h"
#include <QObject>
#include <memory>
#include <optional>

class InventoryService : public QObject {
	Q_OBJECT
public:InventoryService(InventoryDataProvider* dataProvider, QObject* parent = nullptr);
	~InventoryService() override;

	std::optional<Inventory> fromContainer(const QUuid& id) const;

public slots:
	void onSave();
	void onLoad();

private:
	class Private;
	std::unique_ptr<Private> d;
};
