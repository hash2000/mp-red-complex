#include "Game/widgets/inventory/stack_split_widget.h"
#include "Game/widgets/inventory/inventory_item_widget.h"
#include "Game/widgets/inventory/inventory_grid.h"
#include "BaseWidgets/clicable_label.h"
#include <QSlider>
#include <QLineEdit>
#include <QIntValidator>
#include <QLabel>
#include <QHBoxLayout>
#include <QApplication>
#include <QScreen>
#include <QDrag>
#include <QMimeData>
#include <QPainter>
#include <QTimer>
#include <QMouseEvent>

class StackSplitWidget::Private {
public:
	Private(StackSplitWidget* parent)
	: q(parent) {
	}

	StackSplitWidget* q;

	InventoryHandler item;
	int selectedCount = 1;

	QSlider* slider = nullptr;
	QLineEdit* countEditor = nullptr;
	ClickableLabel* iconLabel = nullptr;
	QLabel* nameLabel = nullptr;
	InventoryGrid* grid;

	// Родительский предмет для позиционирования
	QWidget* targetWidget = nullptr;
};


StackSplitWidget::StackSplitWidget(const InventoryHandler& item, InventoryGrid* grid, QWidget* parent)
: d(std::make_unique<Private>(this))
, QWidget(parent) {
	d->item = item;
	d->grid = grid;

	setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_ShowWithoutActivating, false);
	setAttribute(Qt::WA_DeleteOnClose, true);

	setupUi();

	// Устанавливаем начальное значение (половина стека или 1)
	int initial = qMax(1, item.count / 2);
	d->slider->setValue(initial);
	d->countEditor->setText(QString::number(initial));
	d->selectedCount = initial;

	setupEventHandling();

	// Автоматическое закрытие при потере фокуса (через таймер для анимации)
	QTimer::singleShot(100, this, [this]() {
		installEventFilter(this);
		grabMouse(); // Захватываем мышь для отслеживания кликов вне виджета
		});
}

StackSplitWidget::~StackSplitWidget() {
	releaseMouse();
}

int StackSplitWidget::selectedCount() const {
	return d->selectedCount;
}

const InventoryHandler& StackSplitWidget::originalItem() const {
	return d->item;
}

void StackSplitWidget::setupUi() {
	auto* layout = new QHBoxLayout(this);
	layout->setContentsMargins(8, 4, 8, 4);
	layout->setSpacing(6);

	// Слайдер
	d->slider = new QSlider(Qt::Horizontal, this);
	d->slider->setRange(1, d->item.count);
	d->slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	d->slider->setFixedHeight(18);
	connect(d->slider, &QSlider::valueChanged, this, &StackSplitWidget::onSliderChanged);

	// Спинбокс
	d->countEditor = new QLineEdit(this);
	d->countEditor->setValidator(new QIntValidator(1, d->item.count, this));
	d->countEditor->setFixedWidth(50);
	d->countEditor->setAlignment(Qt::AlignCenter);
	d->countEditor->setText(QString::number(d->selectedCount));
	connect(d->countEditor, &QLineEdit::textChanged, this, &StackSplitWidget::onLineEditTextChanged);

	// Иконка для дропа
	d->iconLabel = new ClickableLabel(this);
	d->iconLabel->setFixedSize(24, 24);
	d->iconLabel->setPixmap(d->item.icon.scaled(24, 24, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	d->iconLabel->setCursor(Qt::OpenHandCursor);
	d->iconLabel->setStyleSheet(R"(
        ClickableLabel {
            border: 1px solid #4a5568;
            border-radius: 4px;
            background-color: #2d3748;
        }
        ClickableLabel:hover {
            border-color: #6366f1;
            background-color: #374151;
        }
    )");
	connect(d->iconLabel, &ClickableLabel::clicked, this, &StackSplitWidget::onStartDrag);

	// Название предмета (маленьким шрифтом)
	d->nameLabel = new QLabel(d->item.name, this);
	d->nameLabel->setStyleSheet("font-size: 9px; color: #a0aec0;");
	d->nameLabel->setAlignment(Qt::AlignVCenter);

	layout->addWidget(d->slider, 1);
	layout->addWidget(d->countEditor);
	layout->addWidget(d->iconLabel);
	layout->addWidget(d->nameLabel);

	// Стиль виджета
	setStyleSheet(R"(
        StackSplitWidget {
            background-color: #1a202c;
            border: 1px solid #4a5568;
            border-radius: 6px;
            padding: 2px;
        }
        QSlider::groove:horizontal {
            height: 4px;
            background: #4a5568;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #6366f1;
            border: 2px solid #4f46e5;
            width: 14px;
            height: 14px;
            margin: -5px 0;
            border-radius: 7px;
        }
        QSpinBox {
            background-color: #2d3748;
            color: white;
            border: 1px solid #4a5568;
            border-radius: 4px;
            padding: 2px;
        }
    )");

	adjustSize();
}

void StackSplitWidget::setupEventHandling() {
	// Разрешаем фокус для спинбокса
	d->countEditor->setFocusPolicy(Qt::StrongFocus);
	d->countEditor->installEventFilter(this);

	// Слайдер тоже может получать фокус
	d->slider->setFocusPolicy(Qt::StrongFocus);
	d->slider->installEventFilter(this);

	// Сам виджет получает фокус при показе
	setFocusPolicy(Qt::StrongFocus);
	setFocus();

	// Отслеживаем движения мыши для сброса таймера автозакрытия
	setMouseTracking(true);
	installEventFilter(this);
}

void StackSplitWidget::positionNearWidget(QWidget* targetWidget) {
	d->targetWidget = targetWidget;

	if (!targetWidget) {
		return;
	}

	QPoint targetCenter = targetWidget->mapToGlobal(targetWidget->rect().center());
	QRect screenRect = targetWidget->screen()->availableGeometry();

	// Позиционируем ПОД предметом
	int x = targetCenter.x() - width() / 2;
	int y = targetWidget->mapToGlobal(targetWidget->rect().bottomLeft()).y() + 4;

	// Если не помещается снизу — показываем СВЕРХУ
	if (y + height() > screenRect.bottom()) {
		y = targetWidget->mapToGlobal(targetWidget->rect().topLeft()).y() - height() - 4;
	}

	// Ограничиваем по горизонтали
	if (x < screenRect.left() + 4) x = screenRect.left() + 4;
	if (x + width() > screenRect.right() - 4) x = screenRect.right() - width() - 4;

	move(x, y);
	show();
	raise();
	activateWindow();
	setFocus();
}

void StackSplitWidget::onSliderChanged(int value) {
	d->selectedCount = value;
	d->countEditor->blockSignals(true);
	d->countEditor->setText(QString::number(value));
	d->countEditor->blockSignals(false);
}

void StackSplitWidget::onLineEditTextChanged(const QString& text) {
	bool ok = false;
	int value = text.toInt(&ok);
	if (ok && value >= 1 && value <= d->item.count) {
		d->selectedCount = value;
		d->slider->blockSignals(true);
		d->slider->setValue(value);
		d->slider->blockSignals(false);
	}
}

void StackSplitWidget::onStartDrag() {

	d->iconLabel->setCursor(Qt::ClosedHandCursor);
	const auto splitItem = createSplitItem();

	QDrag* drag = new QDrag(this);
	QMimeData* mimeData = new QMimeData();

	mimeData->setData("application/x-game-item", splitItem.toMimeData());
	mimeData->setData("application/x-game-item-source-inventory-id", d->grid->inventoryId().toUtf8());
	mimeData->setText(splitItem.name);

	// Иконка для курсора
	QPixmap dragPixmap = splitItem.icon.scaled(
		InventoryItemWidget::CELL_SIZE,
		InventoryItemWidget::CELL_SIZE,
		Qt::KeepAspectRatio, Qt::SmoothTransformation);
	drag->setMimeData(mimeData);
	drag->setPixmap(dragPixmap);
	drag->setHotSpot(dragPixmap.rect().center());

	// Визуальная обратная связь
	d->iconLabel->setCursor(Qt::ClosedHandCursor);

	Qt::DropAction dropAction = drag->exec(Qt::MoveAction);

	d->iconLabel->setCursor(Qt::OpenHandCursor);

	if (dropAction == Qt::MoveAction) {
		emit splitDragStarted(splitItem);
		close(); // Закрываем виджет после успешного дропа
	}
}

InventoryHandler StackSplitWidget::createSplitItem() const {
	InventoryHandler splitItem = d->item;
	splitItem.count = d->selectedCount;
	return splitItem;
}

void StackSplitWidget::mousePressEvent(QMouseEvent* event) {
	// Разрешаем взаимодействие только с дочерними элементами
	if (childAt(event->position().toPoint())) {
		QWidget::mousePressEvent(event);
	}
	else {
		// Клик вне активных элементов — отмена
		emit splitCancelled();
		close();
	}
}

void StackSplitWidget::leaveEvent(QEvent* event) {
	QWidget::leaveEvent(event);
	// Не закрываем автоматически — пользователь может выйти за границы для дропа
}

bool StackSplitWidget::eventFilter(QObject* watched, QEvent* event) {
	if (event->type() == QEvent::MouseButtonPress) {
		QMouseEvent* me = static_cast<QMouseEvent*>(event);
		// Если клик вне виджета — отмена
		const auto& thisRect = rect();
		const auto& thisPosition = me->position().toPoint();
		if (!thisRect.contains(thisPosition)) {
			emit splitCancelled();
			close();
			return true;
		}
	}
	else if (event->type() == QEvent::KeyPress) {
		QKeyEvent* ke = static_cast<QKeyEvent*>(event);
		if (ke->key() == Qt::Key_Escape) {
			emit splitCancelled();
			close();
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}
