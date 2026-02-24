#pragma once
#include <QObject>
#include <memory>
#include <optional>

class InventoryService;
class ItemsService;
class InventoryDataProvider;
class InventoryItemMimeData;

class InventoriesService : public QObject {
	Q_OBJECT
public:
	InventoriesService(InventoryDataProvider* dataProvider, ItemsService* itemsService, QObject* parent = nullptr);
	~InventoriesService() override;

	InventoryService* inventoryService(const QUuid& id, bool loadIfNotExists = true) const;
	bool crossInventoryMove(const InventoryItemMimeData& item, int col, int row, const QUuid& fromInventoryId, const QUuid& toInventoryId);

public slots:
	void onSave();
	void onLoad();
	void onInventoryClosed(const QUuid& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
