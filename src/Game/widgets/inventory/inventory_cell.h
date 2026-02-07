#pragma once
#include <QWidget>
#include <QPoint>
#include <memory>

class InventoryItemWidget;

class InventoryCell : public QWidget {
	Q_OBJECT

public:
	explicit InventoryCell(int col, int row, QWidget* parent = nullptr);
	~InventoryCell() override;

	int column() const;
	int row() const;


	void setOccupied(bool occupied);
	bool isOccupied() const;

	void setItemWidget(InventoryItemWidget* widget);
	InventoryItemWidget* itemWidget() const;

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
