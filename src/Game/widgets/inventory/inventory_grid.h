#pragma once
#include <QWidget>
#include <QVector>
#include <QMap>
#include <QUuid>

class InventoryCell;
class InventoryItemWidget;
class InventoriesService;
class InventoryService;
class InventoryItemHandler;
class InventoryItemMimeData;

class InventoryGrid : public QWidget {
	friend class InventoryItemWidget;
	Q_OBJECT
public:
	explicit InventoryGrid(QWidget* parent = nullptr);
	~InventoryGrid() override;

	void setInventoryService(InventoriesService* inventories, const QUuid& id);
	InventoryService* inventoryService() const;

	InventoryItemWidget* widgetAt(int col, int row) const;

	int rows() const;
	int cols() const;

	QString inventoryId() const;

	QString inventoryName() const;

protected:
	QString newObjectName();

signals:
	void itemDroppedForEquipment(const InventoryItemMimeData& item);
	void containerOpened(const InventoryItemMimeData& container);

protected:
	void paintEvent(QPaintEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;

private:
	void onItemPlaced(const InventoryItemMimeData& item, int row, int col);
	void onItemRemoved(const InventoryItemMimeData& item, int row, int col);
	void onItemMoved(const InventoryItemMimeData& item, int oldCol, int oldRow, int newCol, int newRow);
	void onItemCountChanged(const InventoryItemMimeData& item);

	void createWidgetForItem(const InventoryItemMimeData& item);
	void removeWidgetForItem(const InventoryItemMimeData& item);
	void moveWidgetForItem(const InventoryItemMimeData& item, int newCol, int newRow);
	void updateGridSize();
	void showDropPreview(int col, int row, int width, int height, bool canPlace);
	void hideDropPreview();
	void changeItemStackCount(const InventoryItemMimeData& item);

private:
	class Private;
	std::unique_ptr<Private> d;
};
