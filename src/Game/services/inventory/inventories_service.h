#pragma once
#include "Game/services/inventory/inventory_service.h"
#include "DataLayer/inventory/inventory_data_provider.h"
#include <QObject>
#include <memory>
#include <optional>

class InventoriesService : public QObject {
	Q_OBJECT
public:
	InventoriesService(InventoryDataProvider* dataProvider, QObject* parent = nullptr);
	~InventoriesService() override;

	InventoryService* inventoryService(const QUuid& id, bool loadIfNotExists = true) const;
	bool crossInventoryMove(const InventoryHandler& item, int col, int row, const QUuid& fromInventoryId, const QUuid& toInventoryId);

public slots:
	void onSave();
	void onLoad();

private:
	class Private;
	std::unique_ptr<Private> d;
};
