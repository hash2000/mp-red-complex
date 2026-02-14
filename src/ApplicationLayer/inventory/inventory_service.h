#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QObject>
#include <QUuid>
#include <memory>

class InventoriesService;
class InventoriesController;

class InventoryService : public QObject {
	Q_OBJECT

public:
	InventoryService(std::shared_ptr<Inventory> inventory, InventoriesController* controller);
	~InventoryService() override;

	std::shared_ptr<Inventory> inventory() const;
	InventoriesController* controller() const;

	bool placeItem(const InventoryHandler& item);
	bool canPlaceItem(const InventoryHandler& item, int col, int row, bool checkItemPlace) const;
	std::optional<QPoint> findFreeSpace(const InventoryHandler& item, bool checkItemPlace) const;

	bool moveItem(const QString& id, int newCol, int newRow, bool checkItemPlace);
	void removeItem(const InventoryHandler& item);

	bool splitItemStack(const InventoryHandler& item, int newCol, int newRow, bool checkItemPlace);

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
