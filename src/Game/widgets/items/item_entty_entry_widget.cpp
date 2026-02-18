#include "Game/widgets/items/item_entty_entry_widget.h"
#include "Game/widgets/items/items_styles.h"
#include "Game/widgets/items/item_tooltip.h"
#include <QApplication>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPainter>

class ItemEntityEntryWidget::Private {
public:
	Private(ItemEntityEntryWidget* parent)
	: q(parent) {
		if (!s_tooltipInstance) {
			s_tooltipInstance = new ItemTooltip(qApp->activeWindow());
		}
	}

	ItemEntityEntryWidget* q;
	ItemEntity entity;
	QPushButton* createButton;
	QLabel* iconLabel;
	static ItemTooltip* s_tooltipInstance;
};

ItemTooltip* ItemEntityEntryWidget::Private::s_tooltipInstance = nullptr;

ItemEntityEntryWidget::ItemEntityEntryWidget(const ItemEntity& entity, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {

	d->entity = entity;
	setFixedHeight(38);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// Основной горизонтальный лейаут
	auto layout = new QHBoxLayout(this);
	layout->setContentsMargins(12, 6, 12, 6);
	layout->setSpacing(12);

	// Иконка предмета
	d->iconLabel = new QLabel();
	d->iconLabel->setFixedSize(32, 32);
	d->iconLabel->installEventFilter(this);
	d->iconLabel->setScaledContents(true);
	d->iconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 80); border-radius: 4px;");

	if (!entity.icon.isNull()) {
		d->iconLabel->setPixmap(entity.icon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	}
	else {
		// Заглушка для отсутствующей иконки
		QPixmap placeholder(32, 32);
		placeholder.fill(QColor("#1e293b"));
		QPainter painter(&placeholder);
		painter.setPen(QColor("#4a5568"));
		painter.drawText(placeholder.rect(), Qt::AlignCenter, "N/A");
		d->iconLabel->setPixmap(placeholder);
	}

	// Контейнер для текста (две строки)
	auto textContainer = new QWidget();
	auto textLayout = new QVBoxLayout(textContainer);
	textLayout->setContentsMargins(0, 0, 0, 0);
	textLayout->setSpacing(2);

	// ID предмета (зелёный)
	auto idLabel = new QLabel(entity.id);
	idLabel->setStyleSheet(QString("color: #10b981; font-family: 'Consolas', 'Courier New', monospace; font-size: 11px;"));
	idLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	// Название предмета (цвет по редкости)
	auto nameLabel = new QLabel(entity.name);
	nameLabel->setStyleSheet(QString("color: %1; font-weight: 600; font-size: 13px;").arg(ItemsStyles::rarityColor(entity.rarity)));
	nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

	textLayout->addWidget(idLabel);
	textLayout->addWidget(nameLabel);
	textLayout->addStretch();

	// Кнопка "Создать"
	d->createButton = new QPushButton("Создать");
	d->createButton->setFixedSize(80, 28);
	d->createButton->setStyle(new FlatPrimaryButtonStyle());
	d->createButton->setStyleSheet(R"(
            QPushButton {
                background-color: #10b981;
                color: #0f172a;
                border: none;
                border-radius: 4px;
                font-weight: 600;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #0da271;
            }
            QPushButton:pressed {
                background-color: #0a8c62;
            }
        )");

	// Добавляем элементы в лейаут
	layout->addWidget(d->iconLabel);
	layout->addWidget(textContainer);
	layout->addStretch();
	layout->addWidget(d->createButton);

	// Стилизация фона записи
	setStyleSheet(R"(
            ItemEntityEntryWidget {
                border: 2px dashed #4a5568;
                background-color: rgba(30, 41, 59, 200);
                border-radius: 6px;
            }
        )");

	// Подключаем сигнал кнопки
	connect(d->createButton, &QPushButton::clicked, this, [this]() {
		emit createRequested(d->entity.id);
		});
}

ItemEntityEntryWidget::~ItemEntityEntryWidget() = default;

void ItemEntityEntryWidget::paintEvent(QPaintEvent* event) {
	QStyleOption opt;
	opt.initFrom(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
	QWidget::paintEvent(event);
}

void ItemEntityEntryWidget::enterEvent(QEnterEvent* event) {
	QWidget::enterEvent(event);
}

void ItemEntityEntryWidget::leaveEvent(QEvent* event) {
	QWidget::leaveEvent(event);
	// Скрываем тултип при уходе с ВСЕЙ записи
	if (d->s_tooltipInstance && d->s_tooltipInstance->currentItem() == d->entity.id) {
		d->s_tooltipInstance->hideImmediately();
	}
}

bool ItemEntityEntryWidget::eventFilter(QObject* watched, QEvent* event) {
	if (watched == d->iconLabel) {
		if (event->type() == QEvent::Enter) {
			// Показываем тултип с задержкой
			if (d->s_tooltipInstance) {
				d->s_tooltipInstance->showForItem(d->entity, QCursor::pos());
			}
			return true;
		}
		else if (event->type() == QEvent::Leave) {
			if (d->s_tooltipInstance) {
				d->s_tooltipInstance->hideImmediately();
			}
			return true;
		}
	}
	return QWidget::eventFilter(watched, event);
}
