#include "Game/widgets/inventory/inventory_grid.h"
#include "Game/widgets/inventory/inventory_cell.h"
#include "Game/widgets/inventory/inventory_item_widget.h"
#include <QPoint>
#include <QLayoutItem>
#include <QLayout>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPainter>

class InventoryGrid::Private {
public:
	Private(InventoryGrid* parent)
		: q(parent) {
	}

	InventoryGrid* q;
	int rows;
	int cols;
	int cellSize = 32;

	QVector<QVector<InventoryCell*>> cells;
	QHash<QPoint, InventoryItemWidget*> items;
};

InventoryGrid::InventoryGrid(int rows, int cols, QWidget* parent)
	: d(std::make_unique<Private>(this)) {
	d->rows = rows;
	d->cols = cols;
	setFixedSize(d->cols * d->cellSize, d->rows * d->cellSize);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	setupGrid();
}

InventoryGrid::~InventoryGrid() {
	clear();
}

int InventoryGrid::rows() const {
	return d->rows;
}

int InventoryGrid::cols() const {
	return d->cols;
}

bool InventoryGrid::placeItem(const InventoryItem& item, int col, int row) {
	// Проверка границ
	if (col < 0 || row < 0 || col + item.width > d->cols || row + item.height > d->rows) {
		return false;
	}

	// Проверка занятости ВСЕХ ячеек предмета
	for (int dy = 0; dy < item.height; ++dy) {
		for (int dx = 0; dx < item.width; ++dx) {
			if (d->cells[col + dx][row + dy]->isOccupied()) {
				return false;
			}
		}
	}

	auto* widget = new InventoryItemWidget(item, this);
	widget->setFixedSize(item.width * d->cellSize, item.height * d->cellSize);
	widget->move(col * d->cellSize, row * d->cellSize);
	widget->show();
	widget->setGridPosition({ col, row });

	for (int dy = 0; dy < item.height; ++dy) {
		for (int dx = 0; dx < item.width; ++dx) {
			d->cells[col + dx][row + dy]->setOccupied(true);
			if (dx == 0 && dy == 0) {
				d->cells[col][row]->setItemWidget(widget);
			}
		}
	}

	d->items[{col, row}] = widget;

	emit itemPlaced(widget);
	return true;
}

bool InventoryGrid::placeItemWidget(InventoryItemWidget* widget, int col, int row) {
	if (!widget || !canPlaceItem(widget->item(), col, row)) {
		delete widget;
		return false;
	}

	const InventoryItem& item = widget->item();

	// Установка размера и позиции виджета
	widget->setFixedSize(d->cellSize * item.width, d->cellSize * item.height);
	widget->move(col * d->cellSize, row * d->cellSize);
	widget->show();
	widget->setGridPosition(QPoint(col, row));

	// Занятие ячеек
	for (int dy = 0; dy < item.height; ++dy) {
		for (int dx = 0; dx < item.width; ++dx) {
			int cellCol = col + dx;
			int cellRow = row + dy;

			if (dx == 0 && dy == 0) {
				// Ячейка [0,0] — привязываем виджет
				d->cells[cellRow][cellCol]->setItemWidget(widget);
			}
			d->cells[cellRow][cellCol]->setOccupied(true);
		}
	}

	// Сохранение в мапу
	d->items[QPoint(col, row)] = widget;

	// Подключение сигналов для очистки
	connect(widget, &InventoryItemWidget::removedFromGrid, this,
		[this, widget]() { removeItemWidget(widget); });

	emit itemPlaced(widget);
	return true;
}

void InventoryGrid::removeItem(int col, int row) {
	if (col < 0 || row < 0 || col >= d->cols || row >= d->rows) {
		return;
	}

	if (auto widget = d->cells[col][row]->itemWidget()) {
		removeItemWidget(widget);
	}
}

void InventoryGrid::removeItemWidget(InventoryItemWidget* widget) {
	if (!widget) {
		return;
	}

	QPoint pos = widget->gridPosition();
	if (pos.x() < 0 || pos.y() < 0) {
		return; // Не в сетке
	}

	const InventoryItem& item = widget->item();

	// Освобождение ячеек
	for (int dy = 0; dy < item.height; ++dy) {
		for (int dx = 0; dx < item.width; ++dx) {
			int cellCol = pos.x() + dx;
			int cellRow = pos.y() + dy;

			if (cellCol < d->cols && cellRow < d->rows) {
				d->cells[cellRow][cellCol]->setOccupied(false);
				if (dx == 0 && dy == 0) {
					d->cells[cellRow][cellCol]->setItemWidget(nullptr);
				}
			}
		}
	}

	// Удаление из мапы
	d->items.remove(pos);

	// Удаление виджета (асинхронно, чтобы избежать проблем с событиями)
	widget->hide();
	widget->deleteLater();

	emit itemRemoved(widget);
}

std::optional<QPoint> InventoryGrid::findFreeSpace(const InventoryItem& item) const {
	for (int row = 0; row < d->rows; ++row) {
		for (int col = 0; col < d->cols; ++col) {
			if (canPlaceItem(item, col, row)) {
				return QPoint(col, row);
			}
		}
	}

	return std::nullopt;
}

bool InventoryGrid::canPlaceItem(const InventoryItem& item, int col, int row) const {
	if (col < 0 || row < 0 || col + item.width > d->cols || row + item.height > d->rows) {
		return false;
	}

	for (int dy = 0; dy < item.height; ++dy) {
		for (int dx = 0; dx < item.width; ++dx) {
			if (d->cells[col + dx][row + dy]->isOccupied()) {
				return false;
			}
		}
	}
	return true;
}

InventoryItemWidget* InventoryGrid::itemAt(int col, int row) const {
	if (col < 0 || row < 0 || col >= d->cols || row >= d->rows) {
		return nullptr;
	}

	return d->cells[col][row]->itemWidget();
}

void InventoryGrid::clear() {
	auto keys = d->items.keys();
	for (const auto& pos : keys) {
		if (auto widget = d->items.value(pos)) {
			removeItemWidget(widget);
		}
	}

	// Сбрасываем состояние ячеек
	for (int col = 0; col < d->cols; ++col) {
		for (int row = 0; row < d->rows; ++row) {
			d->cells[col][row]->setOccupied(false);
			d->cells[col][row]->setItemWidget(nullptr);
		}
	}

	d->items.clear();
}

InventoryGrid* InventoryGrid::createContainerGrid(const InventoryItemContainerCapacity& capacity) {
	auto grid = new InventoryGrid(capacity.rows, capacity.cols);
	return grid;
}

void InventoryGrid::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	updateGridLayout();
}

void InventoryGrid::setupGrid() {
	d->cells.resize(d->cols);
	for (int col = 0; col < d->cols; ++col) {
		d->cells[col].resize(d->rows);
		for (int row = 0; row < d->rows; ++row) {
			auto* cell = new InventoryCell(col, row, this);
			cell->setGeometry(col * d->cellSize, row * d->cellSize, d->cellSize, d->cellSize);
			d->cells[col][row] = cell;
		}
	}
}

void InventoryGrid::updateGridLayout() {
	if (!layout()) {
		return;
	}

	// Динамический размер ячейки в зависимости от размера виджета
	int cellWidth = width() / d->cols;
	int cellHeight = height() / d->rows;
	d->cellSize = qMin(cellWidth, cellHeight);

	for (int row = 0; row < d->rows; ++row) {
		for (int col = 0; col < d->cols; ++col) {
			d->cells[row][col]->setFixedSize(d->cellSize, d->cellSize);
		}
	}

	// Обновление позиций предметов
	for (auto it = d->items.begin(); it != d->items.end(); ++it) {
		QPoint pos = it.key();
		InventoryItemWidget* widget = it.value();
		if (widget && widget->parentWidget() == this) {
			widget->setFixedSize(d->cellSize * widget->item().width,
				d->cellSize * widget->item().height);
			widget->move(pos.x() * d->cellSize, pos.y() * d->cellSize);
		}
	}
}

void InventoryGrid::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasFormat("application/x-game-item")) {
		event->acceptProposedAction();
	}
	else {
		event->ignore();
	}
}

void InventoryGrid::dragMoveEvent(QDragMoveEvent* event) {
	// Опционально: подсветка ячейки под курсором
	QWidget::dragMoveEvent(event);
}

void InventoryGrid::dropEvent(QDropEvent* event) {
	if (!event->mimeData()->hasFormat("application/x-game-item")) {
		event->ignore();
		return;
	}

	QByteArray data = event->mimeData()->data("application/x-game-item");
	auto itemOpt = InventoryItem::fromMimeData(data);

	if (!itemOpt.has_value()) {
		event->ignore();
		return;
	}

	// Преобразуем позицию курсора в координаты сетки
	QPoint dropPos = event->position().toPoint();
	int col = dropPos.x() / d->cellSize;
	int row = dropPos.y() / d->cellSize;

	// Проверяем точную позицию или ищем свободное место
	if (!canPlaceItem(*itemOpt, col, row)) {
		auto freePos = findFreeSpace(*itemOpt);
		if (freePos.has_value()) {
			col = freePos->x();
			row = freePos->y();
		}
		else {
			event->ignore();
			return;
		}
	}

	if (placeItem(*itemOpt, col, row)) {
		event->acceptProposedAction();
	}
	else {
		event->ignore();
	}
}

void InventoryGrid::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	// Рисуем сетку поверх всех элементов
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);

	// Вертикальные линии
	for (int col = 0; col <= d->cols; ++col) {
		int x = col * d->cellSize;
		painter.setPen((col == d->cols) ? Qt::gray : Qt::darkGray);
		painter.drawLine(x, 0, x, height());
	}

	// Горизонтальные линии
	for (int row = 0; row <= d->rows; ++row) {
		int y = row * d->cellSize;
		painter.setPen((row == d->rows) ? Qt::gray : Qt::darkGray);
		painter.drawLine(0, y, width(), y);
	}
}
