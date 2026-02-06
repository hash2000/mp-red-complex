#include "Game/widgets/equipment/equipment_slot.h"
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QDrag>
#include <QStyle>

class EquipmentSlot::Private {
public:
  Private(EquipmentSlot* parent)
	: q(parent) {
  }

  EquipmentSlot* q;

  SlotType type;
  EquipmentWidget* parentWidget;
  std::optional<Item> item;
  bool highlighted = false;
  bool dragging = false;
};

EquipmentSlot::EquipmentSlot(SlotType type, EquipmentWidget* parentWidget, QWidget* parent)
  : d(std::make_unique<Private>(this))
	, QLabel(parent) {
	setAcceptDrops(true);
	setAlignment(Qt::AlignCenter);
	setFixedSize(64, 64);

	d->type = type;
	d->parentWidget = parentWidget;

	// Специальные размеры для некоторых слотов
	switch (type) {
	case SlotType::WeaponLeft:
	case SlotType::WeaponRight:
		setFixedSize(48, 96); // Вертикальное оружие
		break;
	case SlotType::Body:
		setFixedSize(64, 96); // Тело выше
		break;
	default:
		break;
	}

	setStyleSheet(R"(
        EquipmentSlot {
					border: 2px dashed #4a5568;
					background-color: rgba(30, 41, 59, 200);
					border-radius: 6px;
        }
        EquipmentSlot[highlight="true"] {
					border: 2px solid #ffd700;
        }
        EquipmentSlot[occupied="true"] {
					border: 2px solid #4a5568;
					background-color: rgba(45, 55, 72, 220);
        }
        EquipmentSlot[occupied="true"][rarity="1"] { border-color: #94a3b8; }
        EquipmentSlot[occupied="true"][rarity="2"] { border-color: #6366f1; }
        EquipmentSlot[occupied="true"][rarity="3"] { border-color: #f43f5e; }
    )");

	updateVisualState();
}

EquipmentSlot::~EquipmentSlot() = default;

bool EquipmentSlot::isOccupied() const {
  return d->item.has_value();
}

bool EquipmentSlot::isHighlighted() const {
  return d->highlighted;
}

void EquipmentSlot::setHighlighted(bool highlighted) {
	if (d->highlighted != highlighted) {
		d->highlighted = highlighted;
		updateVisualState();
	}
}

const std::optional<Item>& EquipmentSlot::item() const {
  return d->item;
}

EquipmentSlot::SlotType EquipmentSlot::slotType() const {
  return d->type;
}

bool EquipmentSlot::setItem(const Item& item) {
	if (!canAcceptItem(item)) {
		return false;
	}

	d->item = item;
	setPixmap(item.icon.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	updateVisualState();
	emit itemEquipped(item, this);
	return true;
}

void EquipmentSlot::clearItem() {
	if (d->item.has_value()) {
		Item removed = *d->item;
		d->item.reset();
		clear();
		updateVisualState();
		emit itemRemoved(removed, this);
	}
}

void EquipmentSlot::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		if (d->item.has_value()) {
			startDrag();
		}
		else {
			emit slotClicked(this);
		}
	}
	QLabel::mousePressEvent(event);
}

void EquipmentSlot::dragEnterEvent(QDragEnterEvent* event) {
	if (event->mimeData()->hasFormat("application/x-game-item")) {
		QByteArray itemData = event->mimeData()->data("application/x-game-item");
		QDataStream stream(itemData);
		QString id;
		qint32 typeInt;
		qint32 rarity;
		stream >> id >> typeInt >> rarity;

		Item::Type type = static_cast<Item::Type>(typeInt);
		Item previewItem{ id, "Preview", type, QPixmap(), static_cast<int>(rarity) };

		if (canAcceptItem(previewItem)) {
			setHighlighted(true);
			event->acceptProposedAction();
			return;
		}
	}
	event->ignore();
}

void EquipmentSlot::dragLeaveEvent(QDragLeaveEvent* event) {
	setHighlighted(false);
	QLabel::dragLeaveEvent(event);
}

void EquipmentSlot::dropEvent(QDropEvent* event) {
	setHighlighted(false);

	if (event->mimeData()->hasFormat("application/x-game-item")) {
		QByteArray itemData = event->mimeData()->data("application/x-game-item");
		QDataStream stream(itemData);
		QString id;
		qint32 typeInt;
		qint32 rarity;
		stream >> id >> typeInt >> rarity;

		// Здесь нужно получить полные данные предмета из вашей системы предметов
		// Для примера создаём временный предмет:
		Item droppedItem{
				id,
				QString("Item %1").arg(id),
				static_cast<Item::Type>(typeInt),
				QPixmap(64, 64), // Замените на реальную иконку
				static_cast<int>(rarity)
		};
		droppedItem.icon.fill(Qt::transparent);

		if (setItem(droppedItem)) {
			event->acceptProposedAction();
			return;
		}
	}

	updateVisualState();
	event->ignore();
}

void EquipmentSlot::paintEvent(QPaintEvent* event) {
	QLabel::paintEvent(event);

	// Добавляем визуальные метки типа слота (опционально)
	if (!d->item.has_value() && !d->dragging) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(QColor(100, 116, 139));

		switch (d->type) {
		case SlotType::Head: painter.drawText(rect(), Qt::AlignCenter, "H"); break;
		case SlotType::Body: painter.drawText(rect(), Qt::AlignCenter, "B"); break;
		case SlotType::WeaponLeft: painter.drawText(rect(), Qt::AlignCenter, "L"); break;
		case SlotType::WeaponRight: painter.drawText(rect(), Qt::AlignCenter, "R"); break;
		case SlotType::GlovesLeft: painter.drawText(rect(), Qt::AlignCenter, "GL"); break;
		case SlotType::GlovesRight: painter.drawText(rect(), Qt::AlignCenter, "GR"); break;
		case SlotType::Boots: painter.drawText(rect(), Qt::AlignCenter, "F"); break;
		case SlotType::RingLeft: painter.drawText(rect(), Qt::AlignCenter, "RL"); break;
		case SlotType::RingRight: painter.drawText(rect(), Qt::AlignCenter, "RR"); break;
		case SlotType::Amulet: painter.drawText(rect(), Qt::AlignCenter, "A"); break;
		}
	}
}

bool EquipmentSlot::canAcceptItem(const Item& item) const {
	switch (d->type) {
	case SlotType::Head:
		return item.type == Item::Type::Head;
	case SlotType::Body:
		return item.type == Item::Type::Body;
	case SlotType::WeaponLeft:
		return item.type == Item::Type::Weapon || item.type == Item::Type::Shield;
	case SlotType::WeaponRight:
		return item.type == Item::Type::Weapon; // Только оружие в правой
	case SlotType::GlovesLeft:
	case SlotType::GlovesRight:
		return item.type == Item::Type::Gloves;
	case SlotType::Boots:
		return item.type == Item::Type::Boots;
	case SlotType::RingLeft:
	case SlotType::RingRight:
		return item.type == Item::Type::Ring;
	case SlotType::Amulet:
		return item.type == Item::Type::Amulet;
	default:
		return false;
	}
}

void EquipmentSlot::updateVisualState() {
	setProperty("occupied", d->item.has_value());
	setProperty("highlight", d->highlighted);

	// Для редкости предмета (опционально)
	if (d->item.has_value()) {
		setProperty("rarity", d->item->rarity);
	}
	else {
		setProperty("rarity", -1);
	}

	// Применяем изменения стиля
	style()->unpolish(this);
	style()->polish(this);
	update();
}

void EquipmentSlot::startDrag() {
	if (!d->item.has_value()) {
		return;
	}

	QDrag* drag = new QDrag(this);
	QMimeData* mimeData = new QMimeData();
	QByteArray itemData;
	QDataStream stream(&itemData, QIODevice::WriteOnly);
	stream << d->item->id << static_cast<qint32>(d->item->type) << d->item->rarity;

	mimeData->setData("application/x-game-item", itemData);
	mimeData->setText(d->item->name);
	drag->setMimeData(mimeData);
	drag->setPixmap(d->item->icon.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
	drag->setHotSpot(QPoint(drag->pixmap().width() / 2, drag->pixmap().height() / 2));

	// Визуальная обратная связь: временно очищаем слот во время драга
	QPixmap currentPixmap = (!pixmap().isNull()) ? pixmap() : QPixmap();
	clear();
	d->dragging = true;
	update();

	if (drag->exec(Qt::MoveAction) == Qt::IgnoreAction) {
		// Отмена драга — возвращаем предмет
		if (!currentPixmap.isNull()) {
			setPixmap(currentPixmap);
			d->dragging = false;
			updateVisualState();
		}
	}
	else {
		clearItem();
	}
}

