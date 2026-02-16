#pragma once
#include "DataFormat/proto/inventory_item.h"
#include <QObject>
#include <QUuid>
#include <memory>

class InventoriesService;

class InventoryService : public QObject {
	Q_OBJECT

public:
	InventoryService(std::shared_ptr<Inventory> inventory);
	~InventoryService() override;

	std::shared_ptr<Inventory> inventory() const;

	bool placeItem(const InventoryHandler& item);
	int canPlaceItem(const InventoryHandler& item, int col, int row, bool checkItemPlace) const;
	std::optional<QPoint> findFreeSpace(const InventoryHandler& item, bool checkItemPlace) const;

	bool moveItem(const InventoryHandler& item, int newCol, int newRow, bool checkItemPlace);
	void removeItem(const InventoryHandler& item);

	bool splitStack(const QString& itemId, int newCol, int newRow, int splitCount);

	std::shared_ptr<InventoryItem> itemById(const QString& id) const;
	InventoryItem* itemAt(int col, int row) const;

	// пространство заполнено этим же элементом
	bool containsItem(const InventoryHandler& item) const;

	void clear();

private:
	void setupCells();

signals:
	void placeItemEvent(const InventoryHandler& item, int row, int col);
	void removeItemEvent(const InventoryHandler& item, int row, int col);
	void moveItemEvent(const InventoryHandler& item, int row, int col, int newCol, int newRow);
	void itemCountChanged(const InventoryHandler& item);

private:
	class Private;
	std::unique_ptr<Private> d;
};
