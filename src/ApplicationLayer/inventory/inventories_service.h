#pragma once
#include "ApplicationLayer/inventory/inventory_service.h"
#include "DataLayer/inventory/inventory_data_provider.h"
#include <QObject>
#include <memory>
#include <optional>

class InventoryService;
class InventoriesController;

class InventoriesService : public QObject {
	Q_OBJECT
public:
	InventoriesService(InventoryDataProvider* dataProvider, QObject* parent = nullptr);
	~InventoriesService() override;

	InventoryService* inventoryService(const QUuid& id, bool loadIfNotExists = true) const;
	bool crossInventoryMove(const InventoryHandler& item, int col, int row, const QUuid& fromInventoryId, const QUuid& toInventoryId);

	InventoriesController* controller() const;

public slots:
	void onSave();
	void onLoad();
	void onInventoryClosed(const QUuid& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
