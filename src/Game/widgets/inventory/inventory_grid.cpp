#include "Game/widgets/inventory/inventory_grid.h"
#include "Game/widgets/inventory/inventory_item_widget.h"
#include "Game/widgets/inventory/drop_preview_widget.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventories_service.h"
#include <QPoint>
#include <QLayoutItem>
#include <QLayout>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QPainter>
#include <QDrag>
#include <QDebug>

class InventoryGrid::Private {
public:
	Private(InventoryGrid* parent)
		: q(parent) {
	}

	InventoryGrid* q;
	// сервис текущего инвентаря
	InventoryService* inventory = nullptr;
	// сервис всех инвентарей, для перебрасывания предметов между инвентарями
	InventoriesService* inventories = nullptr;
	// Кэш виджетов по ID предмета
	QHash<QString, InventoryItemWidget*> widgets; 
	DropPreviewWidget* dropPreview = nullptr;

	static constexpr int CELL_SIZE = InventoryItemWidget::CELL_SIZE;
};

QString InventoryGrid::inventoryId() const {
	return d->inventory ? d->inventory->inventory()->id : QString();
}

QString InventoryGrid::inventoryName() const {
	return d->inventory ? d->inventory->inventory()->name : QString();
}

QString InventoryGrid::newObjectName() {
	const auto name = QString("inventory_grid_%1")
		.arg(inventoryId());
	return name;
}

InventoryGrid::InventoryGrid(QWidget* parent)
	: QWidget(parent), d(std::make_unique<Private>(this))
{
	setAcceptDrops(true);
	setAttribute(Qt::WA_NoSystemBackground, false);

	// Создаём оверлей для подсветки дропа
	d->dropPreview = new DropPreviewWidget(Private::CELL_SIZE, this);
	d->dropPreview->hide();
}

InventoryGrid::~InventoryGrid() = default;

InventoryService* InventoryGrid::inventoryService() const {
	return d->inventory;
}

void InventoryGrid::setInventoryService(InventoriesService* inventories, const QUuid& id) {
	auto service = inventories->inventoryService(id, true);

	if (d->inventory == service) {
		return;
	}

	// Отписываемся от старого сервиса
	if (d->inventory) {
		disconnect(d->inventory, nullptr, this, nullptr);
		// Очищаем все виджеты
		for (auto* widget : d->widgets) {
			widget->deleteLater();
		}
		d->widgets.clear();
	}

	d->inventory = service;
	d->inventories = inventories;

	if (d->inventory) {
		// Подписываемся на события сервиса
		connect(d->inventory, &InventoryService::placeItemEvent, this, &InventoryGrid::onItemPlaced);
		connect(d->inventory, &InventoryService::removeItemEvent, this, &InventoryGrid::onItemRemoved);
		connect(d->inventory, &InventoryService::moveItemEvent, this, &InventoryGrid::onItemMoved);

		// Инициализируем виджеты из текущего состояния сервиса
		updateGridSize();
		for (auto item : d->inventory->items()) {
			createWidgetForItem(*item);
		}
	}

	setObjectName(newObjectName());
}

void InventoryGrid::updateGridSize() {
	if (!d->inventory) {
		return;
	}

	int width = d->inventory->inventory()->cols * Private::CELL_SIZE;
	int height = d->inventory->inventory()->rows * Private::CELL_SIZE;
	setFixedSize(width, height);

	// Синхронизируем размер оверлея
	d->dropPreview->setFixedSize(size());
}

void InventoryGrid::onItemPlaced(const InventoryHandler& item, int row, int col) {
	Q_UNUSED(row);
	Q_UNUSED(col);

	// Создаём виджет для нового предмета
	createWidgetForItem(item);
}

void InventoryGrid::onItemRemoved(const InventoryHandler& item, int row, int col) {
	Q_UNUSED(row);
	Q_UNUSED(col);

	// Удаляем виджет предмета
	removeWidgetForItem(item);
}

void InventoryGrid::onItemMoved(const InventoryHandler& item, int oldRow, int oldCol, int newRow, int newCol) {
	Q_UNUSED(oldRow);
	Q_UNUSED(oldCol);

	// Перемещаем существующий виджет
	moveWidgetForItem(item, item.x, item.y);
}

void InventoryGrid::createWidgetForItem(const InventoryHandler& item) {
	if (d->widgets.contains(item.id)) {
		return;
	}

	auto itemPtr = d->inventory->itemById(item.id);
	if (!itemPtr) {
		return;
	}

	auto widget = new InventoryItemWidget(*itemPtr, this, this);
	widget->setFixedSize(item.width * Private::CELL_SIZE, item.height * Private::CELL_SIZE);
	widget->move(item.x * Private::CELL_SIZE, item.y * Private::CELL_SIZE);
	widget->show();

	// Подключаем сигнал удаления (для перемещения между сетками)
	connect(widget, &InventoryItemWidget::removedFromGrid, this, [this, widget](InventoryItemWidget* srcWidget) {
		if (srcWidget != widget || !d->inventory) return;

		// Удаляем из сервиса (сервис сам обновит состояние)
		d->inventory->removeItem(widget->item());
		// Виджет будет удалён в onItemRemoved
		});

	d->widgets[item.id] = widget;
}

void InventoryGrid::removeWidgetForItem(const InventoryHandler& item) {
	if (!d->widgets.contains(item.id)) {
		return;
	}

	auto widget = d->widgets.take(item.id);
	if (widget) {
		widget->hide();
		widget->deleteLater();
	}
}

void InventoryGrid::moveWidgetForItem(const InventoryHandler& item, int newCol, int newRow) {
	if (!d->widgets.contains(item.id)) {
		return;
	}

	auto* widget = d->widgets[item.id];
	if (!widget) {
		return;
	}

	// Обновляем позицию виджета
	widget->move(newCol * Private::CELL_SIZE, newRow * Private::CELL_SIZE);
	widget->setGridPosition({ newCol, newRow });
	widget->raise();
	widget->update();
}

InventoryItemWidget* InventoryGrid::widgetAt(int col, int row) const {
	if (!d->inventory) {
		return nullptr;
	}

	auto item = d->inventory->itemAt(col, row);
	if (!item || !d->widgets.contains(item->id)) {
		return nullptr;
	}

	return d->widgets[item->id];
}

int InventoryGrid::rows() const {
	return d->inventory ? d->inventory->inventory()->rows : 0;
}

int InventoryGrid::cols() const {
	return d->inventory ? d->inventory->inventory()->cols : 0;
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
	if (!d->inventory || !event->mimeData()->hasFormat("application/x-game-item")) {
		event->ignore();
		return;
	}

	auto item = InventoryItem::fromMimeData(event->mimeData()->data("application/x-game-item"));
	const auto sourceInventoryId = QString::fromUtf8(event->mimeData()->data("application/x-game-item-source-inventory-id"));
	const auto currentInventoryId = inventoryId();
	const auto isSameGrid = (sourceInventoryId == currentInventoryId);

	// Определяем целевую ячейку
	QPoint pos = event->position().toPoint();
	int col = pos.x() / Private::CELL_SIZE;
	int row = pos.y() / Private::CELL_SIZE;

	// Проверяем возможность размещения
	bool canPlace = d->inventory->canPlaceItem(item, col, row, isSameGrid);

	// Показываем подсветку
	showDropPreview(col, row, item.width, item.height, canPlace);
	event->acceptProposedAction();
}

void InventoryGrid::dragLeaveEvent(QDragLeaveEvent* event) {
	hideDropPreview();
	QWidget::dragLeaveEvent(event);
}

void InventoryGrid::dropEvent(QDropEvent* event) {
	hideDropPreview();

	if (!d->inventory || !event->mimeData()->hasFormat("application/x-game-item")) {
		event->ignore();
		return;
	}

	QByteArray data = event->mimeData()->data("application/x-game-item");
	const auto item = InventoryItem::fromMimeData(data);

	// Определяем целевую позицию
	QPoint pos = event->position().toPoint();
	int col = pos.x() / Private::CELL_SIZE;
	int row = pos.y() / Private::CELL_SIZE;

	// Получаем источник
	if (!event->mimeData()->hasFormat("application/x-game-item-source-inventory-id")) {
		qDebug() << "Drop inventory item" << item.id << "from unknown inventory container";
		return;
	}

	const auto sourceInventoryId = QString::fromUtf8(event->mimeData()->data("application/x-game-item-source-inventory-id"));
	const auto currentInventoryId = inventoryId();
	const auto isSameGrid = (sourceInventoryId == currentInventoryId);

	// Перемещение внутри той же сетки
	if (isSameGrid && d->inventory->containsItem(item)) {
		if (d->inventory->moveItem(item.id, col, row, true)) {
			// Сервис сам обновит состояние → виджет переместится через сигнал moveItemEvent
			event->acceptProposedAction();
			// Говорим источнику НЕ удалять предмет
			return;
		}
		else {
			// не удалось переместить внутри текущей сетки, выходим
			return;
		}
	}

	// нужно извлечь из инвентаря источника, и переместить в текущий инвентарь
	if (d->inventories && d->inventories->crossInventoryMove(item, col, row,
		QUuid::fromString(sourceInventoryId),
		QUuid::fromString(currentInventoryId))) {
		event->acceptProposedAction();
		return;
	}

	event->ignore();
}

void InventoryGrid::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	if (!d->inventory) {
		return;
	}

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, false);

	// Фон
	painter.fillRect(rect(), QColor(30, 41, 59));

	// Сетка
	for (int col = 0; col <= d->inventory->inventory()->cols; ++col) {
		int x = col * Private::CELL_SIZE;
		painter.setPen(QColor(66, 73, 83));
		painter.drawLine(x, 0, x, height());
	}
	for (int row = 0; row <= d->inventory->inventory()->rows; ++row) {
		int y = row * Private::CELL_SIZE;
		painter.setPen(QColor(66, 73, 83));
		painter.drawLine(0, y, width(), y);
	}
}

void InventoryGrid::showDropPreview(int col, int row, int width, int height, bool canPlace) {
	QRect previewRect(
		col * Private::CELL_SIZE,
		row * Private::CELL_SIZE,
		width * Private::CELL_SIZE,
		height * Private::CELL_SIZE
	);

	// Ограничиваем прямоугольник границами сетки
	previewRect = previewRect.intersected(rect());

	if (!previewRect.isEmpty()) {
		d->dropPreview->showPreview(previewRect, canPlace);
	}
}

void InventoryGrid::hideDropPreview() {
	d->dropPreview->hidePreview();
}

void InventoryGrid::resizeEvent(QResizeEvent* event) {
	QWidget::resizeEvent(event);
	if (d->dropPreview) {
		d->dropPreview->setFixedSize(size());
	}
}
