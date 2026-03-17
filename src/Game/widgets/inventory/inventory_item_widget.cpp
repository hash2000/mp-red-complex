#include "Game/widgets/inventory/inventory_item_widget.h"
#include "Game/widgets/inventory/inventory_grid.h"
#include "Game/widgets/inventory/stack_split_widget.h"
#include "Game/styles/items_styles.h"
#include "Game/dragndrop/drag_event_builder.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDrag>
#include <QMimeData>
#include <QToolTip>
#include <QTimer>
#include <QMouseEvent>


class InventoryItemWidget::Private {
public:
	Private(InventoryItemWidget* parent)
		: q(parent) {
	}

	InventoryItemWidget* q;

	InventoryItemHandler item; // Данные предмета
	InventoryGrid* grid; // Родительская сетка
	QPoint gridPos{ -1, -1 }; // Позиция в сетке (-1,-1 = не размещён)
	QPoint dragStartPos; // Позиция начала драга для порога срабатывания

	QLabel* iconLabel; // Иконка предмета
	QLabel* countLabel; // Счётчик для стеков (может быть nullptr)
};

InventoryItemWidget::InventoryItemWidget(const InventoryItemHandler& item, InventoryGrid* grid, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QFrame(parent) {
	auto layout = new QVBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);
	layout->setSpacing(0);

	d->item = item;
	d->grid = grid;

	setFixedSize(
		item.entity->width * ItemsStyles::CELL_SIZE,
		item.entity->height * ItemsStyles::CELL_SIZE);
	setObjectName(newObjectName());

	// Иконка
	d->iconLabel = new QLabel(this);
	d->iconLabel->setPixmap(item.entity->icon.scaled(
		item.entity->width * ItemsStyles::CELL_SIZE - 4,
		item.entity->height * ItemsStyles::CELL_SIZE - 4,
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation
	));
	d->iconLabel->setAlignment(Qt::AlignCenter);
	layout->addWidget(d->iconLabel, 1);

	// Счётчик для стеков
	if (item.entity->maxStack > 1) {
		d->countLabel = new QLabel(QString::number(item.count), this);
		d->countLabel->setStyleSheet("color: white; font-weight: bold; background-color: rgba(0,0,0,120); border-radius: 6px;");
		d->countLabel->setAlignment(Qt::AlignBottom | Qt::AlignRight);
		layout->addWidget(d->countLabel);
	}
	else {
		d->countLabel = nullptr;
	}

	setStyleSheet("InventoryItemWidget { border: 1px solid #4a5568; border-radius: 3px; }");
	//setAcceptDrops(true);
	//setMouseTracking(true);
	setAttribute(Qt::WA_NoSystemBackground, false); // Разрешаем фон
	setAttribute(Qt::WA_OpaquePaintEvent, true);    // Оптимизация отрисовки
	show();
}

InventoryItemWidget::~InventoryItemWidget() = default;

QString InventoryItemWidget::newObjectName() {
	const auto name = QString("inventory_item_widget_%1_%2")
		.arg(d->item.id.toString())
		.arg(d->grid->inventoryId().toString());
	return name;
}

const InventoryItemHandler& InventoryItemWidget::item() const {
	return d->item;
}

QPoint InventoryItemWidget::gridPosition() const {
	return d->gridPos;
}

void InventoryItemWidget::setGridPosition(const QPoint& pos) {
	d->gridPos = pos;
	d->item.x = pos.x();
	d->item.y = pos.y();
}

void InventoryItemWidget::setCount(int count) {
	if (d->item.entity->maxStack <= 1) {
		return;
	}

	d->item.count = qMin(count, d->item.entity->maxStack);
	if (d->countLabel) {
		d->countLabel->setText(QString::number(d->item.count));
	}
	emit countChanged(d->item.count);
}

int InventoryItemWidget::count() const {
	return d->item.count;
}

bool InventoryItemWidget::isContainer() const {
	return d->item.entity->type == ItemType::Container;
}

bool InventoryItemWidget::isEquipment() const {
	return d->item.entity->type == ItemType::Equipment;
}

bool InventoryItemWidget::isStackable() const {
	return d->item.entity->maxStack > 1;
}

void InventoryItemWidget::startDrag() {
	DragEventBuilder builder(this, ItemMimeData(d->item), *d->item.entity, d->grid->inventoryId());
	setWindowOpacity(0.6);
	Qt::DropAction dropAction = builder.ExecDrag(Qt::MoveAction);
	setWindowOpacity(1.0);
}

void InventoryItemWidget::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		d->dragStartPos = event->pos();
	}
	QWidget::mousePressEvent(event);
}

void InventoryItemWidget::mouseMoveEvent(QMouseEvent* event) {
	if (!(event->buttons() & Qt::LeftButton)) {
		return;
	}

	// Порог для начала драга (предотвращает случайные драги при клике)
	if ((event->pos() - d->dragStartPos).manhattanLength() < QApplication::startDragDistance()) {
		return;
	}

	startDrag();
}

void InventoryItemWidget::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		if (isContainer()) {
			emit containerOpened(d->item);
			event->accept();
			return;
		}

		if (isStackable() && d->item.count > 1) {
			// Создаём и показываем виджет разделения
			auto splitWidget = new StackSplitWidget(d->item, d->grid, nullptr);

			// Позиционируем относительно текущего виджета
			QTimer::singleShot(0, splitWidget, [splitWidget, this]() {
				splitWidget->positionNearWidget(this);
				});

			// Обработка отмены
			connect(splitWidget, &StackSplitWidget::splitCancelled, splitWidget, &QWidget::deleteLater);

			event->accept();
			return;
		}
	}

	QWidget::mouseDoubleClickEvent(event);
}

void InventoryItemWidget::dragEnterEvent(QDragEnterEvent* event) {
	if (!event->mimeData()->hasFormat("application/x-game-item")) {
		event->ignore();
		return;
	}

	auto droppedItem = ItemMimeData::fromMimeData(event->mimeData()->data("application/x-game-item"));
	if (droppedItem.id == d->item.id &&
		d->item.count <= d->item.entity->maxStack) {
		event->acceptProposedAction();
		return;
	}	
}

void InventoryItemWidget::dropEvent(QDropEvent* event) {
	if (d->item.entity->type != ItemType::Resource) {
		event->ignore();
		return;
	}

	auto droppedItem = ItemMimeData::fromMimeData(event->mimeData()->data("application/x-game-item"));
	if (droppedItem.id != d->item.id) {
		event->ignore();
		return;
	}

	// Объединение стеков
	int total = d->item.count + droppedItem.count;
	int added = qMin(total, d->item.entity->maxStack) - d->item.count;

	if (added > 0) {
		setCount(d->item.count + added);
		event->acceptProposedAction();

		// Если предмет полностью поглощён, удаляем его источник
		if (total <= d->item.entity->maxStack) {
			emit removedFromGrid(nullptr); // Сигнал для удаления исходного виджета
		}
	}
	else {
		event->ignore();
	}
}

void InventoryItemWidget::paintEvent(QPaintEvent* event) {
	QWidget::paintEvent(event);

	// Визуальная граница для мульти-ячеечных предметов
	if (d->item.entity->width > 1 || d->item.entity->height > 1) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(QPen(QColor(100, 116, 139, 200), 2));
		painter.drawRect(1, 1, width() - 2, height() - 2);
	}
}

