#include "Game/widgets/inventory/inventory_cell.h"
#include "Game/widgets/inventory/inventory_item_widget.h"
#include <QStyle>
#include <QPainter>

class InventoryCell::Private {
public:
	Private(InventoryCell* parent)
		: q(parent) {
	}

	InventoryCell* q;
	int col;
	int row;
	bool occupied = false;
	InventoryItemWidget* itemWidget = nullptr;
};


InventoryCell::InventoryCell(int col, int row, QWidget* parent)
: d(std::make_unique<Private>(this)) {
	d->col = col;
	d->row = row;

	setFixedSize(32, 32);
	setAttribute(Qt::WA_StyledBackground, true);
	setAttribute(Qt::WA_TransparentForMouseEvents);
}

InventoryCell::~InventoryCell() = default;

int InventoryCell::column() const {
	return d->col;
}

int InventoryCell::row() const {
	return d->row;
}

void InventoryCell::setOccupied(bool occupied) {
	if (d->occupied != occupied) {
		d->occupied = occupied;
		setProperty("occupied", occupied);
		style()->unpolish(this);
		style()->polish(this);
		update();
	}
}

bool InventoryCell::isOccupied() const {
	return d->occupied;
}

void InventoryCell::setItemWidget(InventoryItemWidget* widget) {
	d->itemWidget = widget;
}

InventoryItemWidget* InventoryCell::itemWidget() const {
	return d->itemWidget;
}

void InventoryCell::paintEvent(QPaintEvent* event) {

	QWidget::paintEvent(event);
}
