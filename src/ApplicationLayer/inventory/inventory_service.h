#pragma once
#include "Base/container_view.h"
#include "ApplicationLayer/items_placement_service.h"
#include <QObject>
#include <QUuid>
#include <memory>

class InventoriesService;
class ItemsService;
class Inventory;
class InventoryItem;
class InventoryItemHandler;
class ItemMimeData;

class InventoryService : public QObject, public ItemPlacementAbstraction {
	Q_OBJECT
public:
	using EntityView = decltype(make_deref_view(std::declval<const std::map<QString, std::unique_ptr<InventoryItemHandler>>&>()));
public:
	InventoryService(ItemsService* itemsService);
	~InventoryService() override;

	bool load(const Inventory& inventory);

	QString placementId() const override;
	QString inventoryName() const;

	int rows() const override;
	int cols() const override;

	bool placeItem(const ItemMimeData& item) override;
	int canPlaceItem(const ItemMimeData& item, int col, int row, bool checkItemPlace) const override;
	std::optional<QPoint> findFreeSpace(const ItemMimeData& item, bool checkItemPlace) const override;

	bool moveItem(const ItemMimeData& item, int newCol, int newRow, bool checkItemPlace) override;
	void removeItem(const ItemMimeData& item) override;

	const InventoryItemHandler* itemById(const QString& id) const;
	const InventoryItemHandler* itemAt(int col, int row) const;
	EntityView items() const;

	bool containsItem(const ItemMimeData& item) const override;

	void clear() override;

	bool applyItem(const ItemMimeData& item) override;
	bool removeItemsFromStack(const ItemMimeData& item) override;

signals:
	void placeItemEvent(const ItemMimeData& item, int row, int col);
	void removeItemEvent(const ItemMimeData& item, int row, int col);
	void moveItemEvent(const ItemMimeData& item, int row, int col, int newCol, int newRow);
	void itemCountChanged(const ItemMimeData& item);

private:
	class Private;
	std::unique_ptr<Private> d;
};
