#pragma once
#include "Game/widgets/inventory/inventory_item.h"
#include <QScrollArea>
#include <memory>

class InventoryGrid;

class InventoryGridView : public QScrollArea {
	Q_OBJECT

public:
	explicit InventoryGridView(int rows = 6, int cols = 10, QWidget* parent = nullptr);
	~InventoryGridView() override;

	InventoryGrid* grid() const;

	bool addItem(const InventoryItem& item);

private:
	class Private;
	std::unique_ptr<Private> d;
};
