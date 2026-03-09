#pragma once
#include <QObject>
#include <QUuid>
#include <memory>

class ItemPlacementStore;
class ItemMimeData;
class ItemPlacementService;
class InventoriesDataProvider;
class InventoryDataProvider;
class ItemsService;

class InventoriesService : public QObject {
	Q_OBJECT
public:
	InventoriesService(
		InventoriesDataProvider* inventoriesDataProvider,
		InventoryDataProvider* inventoryDataProvider,
		ItemsService* itemsService,
		QObject* parent = nullptr);

	~InventoriesService() override;

	ItemPlacementService* placementService(const QUuid& id, bool loadIfNotExists) const;

	bool moveItem(const ItemMimeData& item, int col, int row, const QUuid& fromId, const QUuid& toId);

	void load();

signals:
	void itemMoved(const ItemMimeData& item, const QUuid& fromId, const QUuid& toId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
