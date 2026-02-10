#include "Game/widgets/inventory/inventory_grid_view.h"
#include "Game/widgets/inventory/inventory_grid.h"
#include "Game/services/inventory_service/inventory_service.h"

class InventoryGridView::Private {
public:
	Private(InventoryGridView* parent)
		: q(parent) {

	}

	InventoryGridView* q;
	InventoryGrid* grid = nullptr;
	InventoryService* inventoryService;
};

InventoryGridView::InventoryGridView(InventoryService* inventoryService, QWidget* parent)
: d(std::make_unique<Private>(this))
, QScrollArea(parent) {
	d->inventoryService = inventoryService;
	setWidgetResizable(false);
	setAlignment(Qt::AlignLeft | Qt::AlignTop);
	setStyleSheet(R"(
        QScrollArea {
            border: 1px solid #4a5568;
            background-color: #1a202c;
        }
        QScrollBar:vertical {
            border: 1px solid #4a5568;
            background: #2d3748;
            width: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:vertical {
            background: #4a5568;
            min-height: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            border: 1px solid #4a5568;
            background: #2d3748;
            height: 12px;
            margin: 0px 0px 0px 0px;
        }
        QScrollBar::handle:horizontal {
            background: #4a5568;
            min-width: 20px;
            border-radius: 4px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )");

	setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

InventoryGridView::~InventoryGridView() = default;

InventoryGrid* InventoryGridView::grid() const {
	return d->grid;
}

bool InventoryGridView::placeItem(InventoryItem& item) {
	if (auto pos = d->grid->findFreeSpace(item)) {
		item.x = pos->x();
		item.y = pos->y();
		return d->grid->placeItem(item);
	}

	return false;
}

bool InventoryGridView::load(const QUuid& id) {
	if (d->grid) {
		return false;
	}

	const auto inventory = d->inventoryService->fromContainer(id);
	if (!inventory) {
		return false;
	}

	const auto inv = inventory.value();
	d->grid = new InventoryGrid(inv.rows, inv.cols);
	setWidget(d->grid);

	for (const auto item : inv.items) {
		d->grid->placeItem(item);
	}

	return true;
}
