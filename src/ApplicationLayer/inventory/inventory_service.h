#pragma once
#include "Base/container_view.h"
#include <QObject>
#include <QUuid>
#include <memory>

class InventoriesService;
class ItemsService;
class Inventory;
class InventoryItem;
class InventoryItemHandler;
class InventoryItemMimeData;

class InventoryService : public QObject {
	Q_OBJECT
public:
	using EntityView = decltype(make_deref_view(std::declval<const std::map<QString, std::unique_ptr<InventoryItemHandler>>&>()));
public:
	InventoryService(ItemsService* itemsService);
	~InventoryService() override;

	bool load(const Inventory& inventory);

	QString inventoryId() const;
	QString inventoryName() const;
	int rows() const;
	int cols() const;

	bool placeItem(const InventoryItemMimeData& item);
	int canPlaceItem(const InventoryItemMimeData& item, int col, int row, bool checkItemPlace) const;
	std::optional<QPoint> findFreeSpace(const InventoryItemMimeData& item, bool checkItemPlace) const;

	bool moveItem(const InventoryItemMimeData& item, int newCol, int newRow, bool checkItemPlace);
	void removeItem(const InventoryItemMimeData& item);

	const InventoryItemHandler* itemById(const QString& id) const;
	const InventoryItemHandler* itemAt(int col, int row) const;
	EntityView items() const;

	bool containsItem(const InventoryItemMimeData& item) const;

	void clear();

	bool applyItem(const InventoryItemMimeData& item);
	bool changeItemsCount(const InventoryItemMimeData& item);

signals:
	void placeItemEvent(const InventoryItemMimeData& item, int row, int col);
	void removeItemEvent(const InventoryItemMimeData& item, int row, int col);
	void moveItemEvent(const InventoryItemMimeData& item, int row, int col, int newCol, int newRow);
	void itemCountChanged(const InventoryItemMimeData& item);

private:
	class Private;
	std::unique_ptr<Private> d;
};
