#include "Game/widgets/items/items_widget.h"
#include "Game/widgets/items/item_entty_entry_widget.h"
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class ItemsWidget::Private {
public:
	Private(ItemsWidget* parent)
		: q(parent) {
	}

	ItemsWidget* q;
	QScrollArea* scrollArea;
	QWidget* contentWidget;
	QVBoxLayout* contentLayout;
	ItemsService* service;
};

ItemsWidget::ItemsWidget(ItemsService* service, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent) {

	d->service = service;

	// Основной вертикальный лейаут
	auto mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);

	// Область прокрутки
	d->scrollArea = new QScrollArea();
	d->scrollArea->setWidgetResizable(true);
	d->scrollArea->setFrameShape(QFrame::NoFrame);
	d->scrollArea->setStyleSheet(R"(
            QScrollArea {
                background-color: #0f172a;
                border: none;
            }
            QScrollBar:vertical {
                border: none;
                background: #1e293b;
                width: 12px;
                margin: 0px;
            }
            QScrollBar::handle:vertical {
                background: #4a5568;
                min-height: 30px;
                border-radius: 6px;
            }
            QScrollBar::handle:vertical:hover {
                background: #64748b;
            }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
                height: 0px;
            }
        )");

	// Контент для прокрутки
	d->contentWidget = new QWidget();
	d->contentLayout = new QVBoxLayout(d->contentWidget);
	d->contentLayout->setContentsMargins(10, 10, 10, 10);
	d->contentLayout->setSpacing(8);
	d->contentLayout->addStretch(); // Для выравнивания элементов сверху

	d->contentWidget->setStyleSheet("background-color: transparent;");
	d->scrollArea->setWidget(d->contentWidget);

	mainLayout->addWidget(d->scrollArea);
}

ItemsWidget::~ItemsWidget() {
	clear();
}

void ItemsWidget::addItemEntity(const ItemEntity& entity) {
	// Убираем stretch перед добавлением нового элемента
	if (d->contentLayout->count() > 0) {
		delete d->contentLayout->takeAt(d->contentLayout->count() - 1);
	}

	auto entry = new ItemEntityEntryWidget(entity, this);
	d->contentLayout->addWidget(entry);

	// Возвращаем stretch в конец
	d->contentLayout->addStretch();

	// Подключаем сигнал записи к сигналу окна
	connect(entry, &ItemEntityEntryWidget::createRequested,
		this, &ItemsWidget::itemCreateRequested);
}

void ItemsWidget::clear() {
	QLayoutItem* item;
	while ((item = d->contentLayout->takeAt(0)) != nullptr) {
		if (item->widget()) {
			delete item->widget();
		}
		delete item;
	}

	d->contentLayout->addStretch();
}
