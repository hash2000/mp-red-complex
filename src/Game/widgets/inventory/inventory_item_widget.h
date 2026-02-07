#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QFrame>
#include <memory>

class InventoryGrid;
class QLabel;

class InventoryItemWidget : public QFrame {
	Q_OBJECT

public:
	explicit InventoryItemWidget(const InventoryItem& item, InventoryGrid* grid, QWidget* parent = nullptr);
	~InventoryItemWidget() override;

	const InventoryItem& item() const;

	// Позиция в сетке (координаты ячейки [0,0] предмета)
	QPoint gridPosition() const;
	void setGridPosition(const QPoint& pos);

	// Для стекируемых предметов
	void setCount(int count);
	int count() const;

	// Проверка типа предмета
	bool isContainer() const;
	bool isEquipment() const;
	bool isStackable() const;

	// Инициировать перетаскивание предмета
	void startDrag();

	// Открыть контейнер (для ящиков)
	void openContainer();

signals:
	void removedFromGrid(InventoryItemWidget* widget);
	void countChanged(int newCount);

protected:
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void dragEnterEvent(QDragEnterEvent* event) override;
	void dropEvent(QDropEvent* event) override;
	void paintEvent(QPaintEvent* event) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
