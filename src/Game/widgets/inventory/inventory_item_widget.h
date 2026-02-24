#pragma once
#include "Game/widgets/items/items_styles.h"
#include <QFrame>

class InventoryGrid;
class InventoryItemHandler;
class InventoryItemWidget;

class InventoryItemWidget : public QFrame {
	Q_OBJECT
public:
	static constexpr int CELL_SIZE = ItemsStyles::CELL_SIZE;

public:
	explicit InventoryItemWidget(const InventoryItemHandler& item, InventoryGrid* grid, QWidget* parent = nullptr);
	~InventoryItemWidget() override;

	const InventoryItemHandler& item() const;

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

	QString newObjectName();

private:
	class Private;
	std::unique_ptr<Private> d;
};
