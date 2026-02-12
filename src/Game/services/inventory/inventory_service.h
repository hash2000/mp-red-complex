#pragma once
#include <QObject>
#include "DataLayer/inventory/inventory_item.h"
#include <memory>

class InventoryService : public QObject {
	Q_OBJECT

public:
	static const int CELL_SIZE = 32;

public:
	InventoryService(std::shared_ptr<Inventory> inventory);
	~InventoryService() override;

	std::shared_ptr<Inventory> inventory() const;

	bool placeItem(const InventoryHandler& item);
	bool canPlaceItem(const InventoryHandler& item, int col, int row) const;
	std::optional<QPoint> findFreeSpace(const InventoryHandler& item) const;

	bool moveItem(const QString& id, int newCol, int newRow);
	void removeItem(const InventoryHandler& item);

	std::shared_ptr<InventoryItem> itemById(const QString& id) const;
	InventoryItem* itemAt(int col, int row) const;
	QVector<InventoryItem*> items() const;

	// пространство заполнено этим же элементом
	bool containsItem(const InventoryHandler& item) const;

	void clear();

private:
	void setupCells();

signals:
	void placeItemEvent(const InventoryHandler& item, int row, int col);
	void removeItemEvent(const InventoryHandler& item, int row, int col);
	void moveItemEvent(const InventoryHandler& item, int row, int col, int newCol, int newRow);

private:
	class Private;
	std::unique_ptr<Private> d;
};
