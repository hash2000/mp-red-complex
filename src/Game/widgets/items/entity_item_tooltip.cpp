#include "Game/widgets/items/entity_item_tooltip.h"
#include "Game/styles/items_styles.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QScreen>
#include <QGuiApplication>

class EntityItemTooltip::Private {
public:
	Private(EntityItemTooltip* parent)
		: q(parent) {
	}

	EntityItemTooltip* q;

	QLabel* iconLabel;
	QLabel* nameLabel;
	QLabel* descriptionLabel;
	QLabel* detailsLabel;
	QTimer* showTimer;
	QString currentItem;
	QBasicTimer animationTimer;
	qreal opacity = 0.0;

	void setupContent(const ItemEntity& item) {

		// Иконка
		if (!item.icon.isNull()) {
			iconLabel->setPixmap(item.icon.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
		else {
			QPixmap placeholder(64, 64);
			placeholder.fill(QColor("#1e293b"));
			QPainter p(&placeholder);
			p.setPen(QColor("#4a5568"));
			p.drawText(placeholder.rect(), Qt::AlignCenter, "N/A");
			iconLabel->setPixmap(placeholder);
		}

		// Название с цветом по редкости
		QString rarityColor = ItemsStyles::rarityColor(item.rarity); // из предыдущего кода
		nameLabel->setText(QString("<span style='color:%1; font-size:16px; font-weight:600;'>%2</span>")
			.arg(rarityColor, item.name));

		// Описание
		descriptionLabel->setText(item.description.isEmpty() ? "<i>Без описания</i>" : item.description);

		// Детали в зависимости от типа
		QString details;
		switch (item.type) {
		case ItemType::Resource: {
			if (item.equipmentType == ItemEquipmentType::Resource) {
				details = "<b>Тип:</b> Сырьё";
			}
			break;
		}
		case ItemType::Equipment: {
			QString slotName;
			switch (item.equipmentType) {
			case ItemEquipmentType::Head: slotName = "Голова"; break;
			case ItemEquipmentType::Body: slotName = "Тело"; break;
			case ItemEquipmentType::Weapon: slotName = "Оружие"; break;
			case ItemEquipmentType::Shield: slotName = "Щит"; break;
			case ItemEquipmentType::Gloves: slotName = "Перчатки"; break;
			case ItemEquipmentType::Boots: slotName = "Обувь"; break;
			case ItemEquipmentType::Ring: slotName = "Кольцо"; break;
			case ItemEquipmentType::Amulet: slotName = "Амулет"; break;
			case ItemEquipmentType::Consumable: slotName = "Расходник"; break;
			default: slotName = "Неизвестно";
			}
			details = QString("<b>Экипировка:</b> %1").arg(slotName);
		}
		break;
		case ItemType::Recipe: {
			if (item.recipe && !item.recipe->ingredients.empty()) {
				QStringList ingredients;
				for (const auto& ing : item.recipe->ingredients) {
					ingredients << QString("• %1 × %2").arg(ing.itemId).arg(ing.amount);
				}
				details = QString("<b>Ингредиенты:</b><br/>%1").arg(ingredients.join("<br/>"));
			}
			else {
				details = "<b>Рецепт:</b> без ингредиентов";
			}
			break;
		}
		case ItemType::Component:
			details = "<b>Тип:</b> Компонент";
			break;
		case ItemType::Container:
			if (item.container.has_value()) {
				details = QString("<b>Тип:</b> Контейнер (%1×%2 ячеек)")
					.arg(item.container->rows)
					.arg(item.container->cols);
			}
			break;
		}

		// Размер стека
		if (item.maxStack > 1) {
			if (!details.isEmpty()) details += "<br/>";
			details += QString("<b>Стек:</b> до %1").arg(item.maxStack);
		}

		detailsLabel->setText(details.isEmpty() ? "" : details);

		// Применяем стили с акцентом по редкости
		QString border = ItemsStyles::borderColor(item.rarity);
		QString bg = ItemsStyles::backgroundColor(item.rarity);

		q->setStyleSheet(QString(R"(
            EntityItemTooltip {
                background: transparent;
            }
            #tooltipContainer {
                background: %1;
                border: 2px solid %2;
                border-radius: 10px;
                padding: 2px;
                min-width: 300px;
                max-width: 400px;
            }
            #itemName {
                color: %3;
                padding-bottom: 4px;
                border-bottom: 1px solid rgba(255, 255, 255, 30);
            }
            #itemDescription {
                color: #cbd5e1;
                font-size: 13px;
                padding: 6px 0;
            }
            #itemDetails {
                color: #94a3b8;
                font-size: 12px;
                background-color: rgba(0, 0, 0, 30);
                padding: 8px;
                border-radius: 6px;
            }
        )").arg(bg, border, rarityColor));
	}

	void positionNear(const QPoint& globalPos) {
		// Размеры тултипа после обновления контента
		q->adjustSize();
		QSize size = q->size();

		// Определяем экран, на котором находится курсор
		QScreen* screen = QGuiApplication::screenAt(globalPos);
		if (!screen) screen = QGuiApplication::primaryScreen();
		QRect screenGeom = screen->availableGeometry();

		// Позиционируем справа и ниже курсора
		int x = globalPos.x() + 12;
		int y = globalPos.y() + 12;

		// Коррекция, если выходим за правую границу
		if (x + size.width() > screenGeom.right()) {
			x = globalPos.x() - size.width() - 12;
		}

		// Коррекция, если выходим за нижнюю границу
		if (y + size.height() > screenGeom.bottom()) {
			y = globalPos.y() - size.height() - 12;
		}

		// Коррекция, если выходим за левую границу
		if (x < screenGeom.left()) {
			x = screenGeom.left() + 8;
		}

		// Коррекция, если выходим за верхнюю границу
		if (y < screenGeom.top()) {
			y = screenGeom.top() + 8;
		}

		q->move(x, y);
	}
};

EntityItemTooltip::EntityItemTooltip(QWidget* parent)
	: d(std::make_unique<Private>(this))
	, QWidget(parent, Qt::ToolTip) {
	setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);
	setAttribute(Qt::WA_TranslucentBackground);
	setAttribute(Qt::WA_ShowWithoutActivating);
	setContentsMargins(0, 0, 0, 0);

	// Основной контейнер с отступами
	auto container = new QWidget(this);
	container->setObjectName("tooltipContainer");
	auto layout = new QVBoxLayout(container);
	layout->setContentsMargins(16, 14, 16, 14);
	layout->setSpacing(10);

	// Иконка + основная информация
	auto headerLayout = new QHBoxLayout();
	headerLayout->setSpacing(14);

	// Иконка (увеличенная)
	d->iconLabel = new QLabel();
	d->iconLabel->setScaledContents(true);
	d->iconLabel->setStyleSheet("background-color: rgba(0, 0, 0, 100); border-radius: 6px;");

	// Правая колонка: название + описание
	auto infoLayout = new QVBoxLayout();
	infoLayout->setSpacing(4);

	d->nameLabel = new QLabel();
	d->nameLabel->setObjectName("itemName");
	d->nameLabel->setWordWrap(true);

	d->descriptionLabel = new QLabel();
	d->descriptionLabel->setObjectName("itemDescription");
	d->descriptionLabel->setWordWrap(true);
	d->descriptionLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

	infoLayout->addWidget(d->nameLabel);
	infoLayout->addWidget(d->descriptionLabel);
	infoLayout->addStretch();

	headerLayout->addWidget(d->iconLabel);
	headerLayout->addLayout(infoLayout);

	// Детали (тип, характеристики, ингредиенты)
	d->detailsLabel = new QLabel();
	d->detailsLabel->setObjectName("itemDetails");
	d->detailsLabel->setWordWrap(true);
	d->detailsLabel->setTextFormat(Qt::RichText);

	layout->addLayout(headerLayout);
	layout->addWidget(d->detailsLabel);

	// Устанавливаем container как центральный виджет
	auto mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->addWidget(container);

	// Задержка перед показом (как в нативных тултипах)
	d->showTimer = new QTimer(this);
	d->showTimer->setSingleShot(true);
	d->showTimer->setInterval(400); // 400ms задержка
	connect(d->showTimer, &QTimer::timeout, this, &EntityItemTooltip::showAnimated);

	// Анимация появления
	setWindowOpacity(0.0);
}

EntityItemTooltip::~EntityItemTooltip() = default;

void EntityItemTooltip::showForItem(const ItemEntity& item, const QPoint& globalPos) {
	d->iconLabel->setFixedSize(
		item.width * ItemsStyles::CELL_SIZE,
		item.height * ItemsStyles::CELL_SIZE);

	// Настраиваем содержимое
	d->setupContent(item);

	// Позиционируем с учётом границ экрана
	d->positionNear(globalPos);

	// Сбрасываем таймер и запускаем задержку
	d->showTimer->stop();
	d->showTimer->start();

	d->currentItem = item.id;
}

void EntityItemTooltip::hideImmediately() {
	d->showTimer->stop();
	hide();
	setWindowOpacity(0.0);
}

QString EntityItemTooltip::currentItem() const {
	return d->currentItem;
}


void EntityItemTooltip::showAnimated() {
	// Плавное появление
	setWindowOpacity(0.0);
	show();
	d->animationTimer.start(16, this); // ~60 FPS
	d->opacity = 0.0;
}

void EntityItemTooltip::timerEvent(QTimerEvent* event) {
	if (event->timerId() == d->animationTimer.timerId()) {
		d->opacity += 0.15;
		if (d->opacity >= 1.0) {
			d->opacity = 1.0;
			d->animationTimer.stop();
		}
		setWindowOpacity(d->opacity);
	}
	else {
		QWidget::timerEvent(event);
	}
}
