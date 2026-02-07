#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QWidget>
#include <QVector>
#include <QMap>

class InventoryCell;
class InventoryItemWidget;

// Управление размещением предметов в сетке
class InventoryGrid : public QWidget {
	friend class InventoryItemWidget;
	Q_OBJECT
public:
	explicit InventoryGrid(int rows = 6, int cols = 10, QWidget* parent = nullptr);
	~InventoryGrid() override;

	int rows() const;
	int cols() const;

	// Размещение предмета
	bool placeItem(const InventoryItem& item, int col, int row);
	bool placeItemWidget(InventoryItemWidget* widget, int col, int row);

	// Удаление предмета
	void removeItem(int col, int row);
	void removeItemWidget(InventoryItemWidget* widget);

	// Поиск свободного места
	std::optional<QPoint> findFreeSpace(const InventoryItem& item) const;

	// Проверка возможности размещения
	bool canPlaceItem(const InventoryItem& item, int col, int row) const;

	// Получение предмета по позиции
	InventoryItemWidget* itemAt(int col, int row) const;

	// Очистка всей сетки
	void clear();

	// Для ящиков — получение внутренней сетки
	static InventoryGrid* createContainerGrid(const InventoryItemContainerCapacity& capacity);

signals:
	void itemPlaced(InventoryItemWidget* widget);
	void itemRemoved(InventoryItemWidget* widget);

protected:
	void resizeEvent(QResizeEvent* event) override;
	void paintEvent(QPaintEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dragMoveEvent(QDragMoveEvent* event) override;
	void dropEvent(QDropEvent* event) override;

private:
	void setupGrid();
	void updateGridLayout();

private:
	class Private;
	std::unique_ptr<Private> d;
};
