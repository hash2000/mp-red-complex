#pragma once
#include "DataLayer/inventory/inventory_item.h"
#include <QScrollArea>
#include <QUuid>
#include <memory>

class InventoryGrid;
class InventoriesService;

class InventoryGridView : public QScrollArea {
	Q_OBJECT

public:
	explicit InventoryGridView(InventoriesService* service, QWidget* parent = nullptr);
	~InventoryGridView() override;

	InventoryGrid* grid() const;

	bool load(const QUuid& id);

private:
	class Private;
	std::unique_ptr<Private> d;
};
