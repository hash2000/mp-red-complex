#include "Game/widgets/inventory/inventory_item_widget.h"
#include "Game/widgets/inventory/inventory_grid.h"
#include <QApplication>
#include <QVBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QDrag>
#include <QMimeData>
#include <QToolTip>
#include <QTimer>


class InventoryItemWidget::Private {
public:
	Private(InventoryItemWidget* parent)
		: q(parent) {
	}

	InventoryItemWidget* q;

	InventoryItem item; // Данные предмета
	InventoryGrid* grid; // Родительская сетка
	QPoint gridPos{ -1, -1 }; // Позиция в сетке (-1,-1 = не размещён)
	QPoint dragStartPos; // Позиция начала драга для порога срабатывания

	QLabel* iconLabel; // Иконка предмета
	QLabel* countLabel; // Счётчик для стеков (может быть nullptr)
};

InventoryItemWidget::InventoryItemWidget(const InventoryItem& item, InventoryGrid* grid, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QFrame(parent) {
	// Устанавливаем ФИКСИРОВАННЫЙ размер на основе ячеек
	setFixedSize(item.width * CELL_SIZE, item.height * CELL_SIZE);
	setObjectName(newObjectName());

	// Создаем контейнер для иконки и счётчика
	auto* layout = new QVBoxLayout(this);
	layout->setContentsMargins(2, 2, 2, 2);
	layout->setSpacing(0);

	d->item = item;
	d->grid = grid;

	// Иконка
	d->iconLabel = new QLabel(this);
	d->iconLabel->setPixmap(item.icon.scaled(
		item.width * CELL_SIZE - 4,
		item.height * CELL_SIZE - 4,
		Qt::KeepAspectRatio,
		Qt::SmoothTransformation
	));
	d->iconLabel->setAlignment(Qt::AlignCenter);
	layout->addWidget(d->iconLabel, 1);

	// Счётчик для стеков
	if (item.maxStack > 1) {
		d->countLabel = new QLabel(QString::number(item.count), this);
		d->countLabel->setStyleSheet("color: white; font-weight: bold; background-color: rgba(0,0,0,120); border-radius: 6px;");
		d->countLabel->setAlignment(Qt::AlignBottom | Qt::AlignRight);
		layout->addWidget(d->countLabel);
	}
	else {
		d->countLabel = nullptr;
	}

	setStyleSheet("InventoryItemWidget { border: 1px solid #4a5568; border-radius: 3px; }");
	setAcceptDrops(true);
	//setMouseTracking(true);
	setAttribute(Qt::WA_NoSystemBackground, false); // Разрешаем фон
	setAttribute(Qt::WA_OpaquePaintEvent, true);    // Оптимизация отрисовки
	show();
}

InventoryItemWidget::~InventoryItemWidget() = default;

QString InventoryItemWidget::newObjectName() {
	static int s_widgetNumber = 0;
	QString name; name.setNum(s_widgetNumber++); name.prepend("inventory_item_widget_");
	return name;
}

const InventoryItem& InventoryItemWidget::item() const {
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
	if (d->item.maxStack <= 1) {
		return;
	}

	d->item.count = qMin(count, d->item.maxStack);
	if (d->countLabel) {
		d->countLabel->setText(QString::number(d->item.count));
	}
	emit countChanged(d->item.count);
}

int InventoryItemWidget::count() const {
	return d->item.count;
}

bool InventoryItemWidget::isContainer() const {
	return d->item.type == InventoryItemType::Container;
}

bool InventoryItemWidget::isEquipment() const {
	return d->item.type == InventoryItemType::Equipment;
}

bool InventoryItemWidget::isStackable() const {
	return d->item.maxStack > 1;
}

void InventoryItemWidget::startDrag() {
	QDrag* drag = new QDrag(this);
	QMimeData* mimeData = new QMimeData();

	mimeData->setData("application/x-game-item", d->item.toMimeData());
	mimeData->setData("application/x-game-item-source-grid", d->grid->objectName().toUtf8());
	mimeData->setData("application/x-game-item-source-widget", objectName().toUtf8());
	mimeData->setData("application/x-game-item-remove-from-source", "1");
	mimeData->setText(d->item.name);

	// Иконка для курсора
	QPixmap dragPixmap;
	if (d->item.icon.isNull()) {
		dragPixmap = QPixmap(CELL_SIZE, CELL_SIZE);
		dragPixmap.fill(Qt::darkCyan);
	}
	else
	{
		dragPixmap = d->item.icon.scaled(CELL_SIZE, CELL_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
	}

	drag->setMimeData(mimeData);
	drag->setPixmap(dragPixmap);
	drag->setHotSpot(QPoint(dragPixmap.width() / 2, dragPixmap.height() / 2));

	// Визуальная обратная связь: полупрозрачность во время драга
	setWindowOpacity(0.6);

	Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

	// Восстанавливаем прозрачность
	setWindowOpacity(1.0);


	if (dropAction == Qt::MoveAction) {		
		const auto removeFrpmSourceFlag = mimeData->data("application/x-game-item-remove-from-source");
		if (removeFrpmSourceFlag == "1") {
			emit removedFromGrid(this);
		}
	}
}

void InventoryItemWidget::openContainer() {
	if (!isContainer() || !d->grid) {
		return;
	}

	// Сигнал вверх по иерархии для открытия контейнера
	QWidget* parent = parentWidget();
	while (parent && !parent->inherits("InventoryItemWidget")) {
		parent = parent->parentWidget();
	}

	if (parent) {
		QMetaObject::invokeMethod(parent, "openContainer",
			Qt::DirectConnection,
			Q_ARG(InventoryItem, d->item)
		);
	}
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
	if (event->button() == Qt::LeftButton && isContainer()) {
		openContainer();
	}
	QWidget::mouseDoubleClickEvent(event);
}

void InventoryItemWidget::dragEnterEvent(QDragEnterEvent* event) {
	if (!event->mimeData()->hasFormat("application/x-game-item")) {
		event->ignore();
		return;
	}

	auto droppedItemOpt = InventoryItem::fromMimeData(event->mimeData()->data("application/x-game-item"));

	if (droppedItemOpt.has_value() && droppedItemOpt->id == d->item.id &&
		d->item.count <= d->item.maxStack) {
		event->acceptProposedAction();
		return;
	}
	
}

void InventoryItemWidget::dropEvent(QDropEvent* event) {
	if (d->item.type != InventoryItemType::Resource) {
		event->ignore();
		return;
	}

	auto droppedItemOpt = InventoryItem::fromMimeData(
		event->mimeData()->data("application/x-game-item")
	);

	if (!droppedItemOpt.has_value() || droppedItemOpt->id != d->item.id) {
		event->ignore();
		return;
	}

	// Объединение стеков
	int total = d->item.count + droppedItemOpt->count;
	int added = qMin(total, d->item.maxStack) - d->item.count;

	if (added > 0) {
		setCount(d->item.count + added);
		event->acceptProposedAction();

		// Если предмет полностью поглощён, удаляем его источник
		if (total <= d->item.maxStack) {
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
	if (d->item.width > 1 || d->item.height > 1) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(QPen(QColor(100, 116, 139, 200), 2));
		painter.drawRect(1, 1, width() - 2, height() - 2);
	}
}

