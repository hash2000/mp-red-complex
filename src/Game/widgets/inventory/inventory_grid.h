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
class ItemMimeData;

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

	QUuid inventoryId() const;

	QString inventoryName() const;

protected:
	QString newObjectName();

signals:
	void itemDroppedForEquipment(const ItemMimeData& item);
	void containerOpened(const ItemMimeData& container);

protected:
	void paintEvent(QPaintEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dragLeaveEvent(QDragLeaveEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;

private:
	void onItemPlaced(const ItemMimeData& item, int col, int row);
	void onItemRemoved(const ItemMimeData& item, int col, int row);
	void onItemMoved(const ItemMimeData& item, int oldCol, int oldRow, int newCol, int newRow);
	void onItemCountChanged(const ItemMimeData& item);

	void createWidgetForItem(const ItemMimeData& item);
	void removeWidgetForItem(const ItemMimeData& item);
	void moveWidgetForItem(const ItemMimeData& item, int newCol, int newRow);
	void updateGridSize();
	void showDropPreview(int col, int row, int width, int height, bool canPlace);
	void hideDropPreview();
	void changeItemStackCount(const ItemMimeData& item);

private:
	class Private;
	std::unique_ptr<Private> d;
};
