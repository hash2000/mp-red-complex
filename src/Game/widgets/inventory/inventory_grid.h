#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QWidget>
#include <QVector>
#include <QMap>

class InventoryCell;
class InventoryItemWidget;
class InventoryService;

class InventoryGrid : public QWidget {
	friend class InventoryItemWidget;
	Q_OBJECT
public:
	explicit InventoryGrid(QWidget* parent = nullptr);
	~InventoryGrid() override;

	void setInventoryService(InventoryService* service);
	InventoryService* inventoryService() const;

	InventoryItemWidget* widgetAt(int col, int row) const;

	int rows() const;
	int cols() const;

	QString inventoryId() const;

protected:
	QString newObjectName();

signals:
	void itemDroppedForEquipment(const InventoryItem& item);
	void containerOpened(const InventoryItem& container);

protected:
	void paintEvent(QPaintEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	void onItemPlaced(const InventoryHandler& item, int row, int col);
	void onItemRemoved(const InventoryHandler& item, int row, int col);
	void onItemMoved(const InventoryHandler& item, int oldRow, int oldCol, int newRow, int newCol);

	void createWidgetForItem(const InventoryHandler& item);
	void removeWidgetForItem(const InventoryHandler& item);
	void moveWidgetForItem(const InventoryHandler& item, int newCol, int newRow);
	void updateGridSize();
	void showDropPreview(int col, int row, int width, int height, bool canPlace);
	void hideDropPreview();

private:
	class Private;
	std::unique_ptr<Private> d;
};
