#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QScrollArea>
#include <QUuid>
#include <memory>

class InventoryGrid;
class InventoryService;

class InventoryGridView : public QScrollArea {
	Q_OBJECT

public:
	explicit InventoryGridView(InventoryService* inventoryService, QWidget* parent = nullptr);
	~InventoryGridView() override;

	InventoryGrid* grid() const;

	bool placeItem(InventoryItem& item);

	bool load(const QUuid& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
