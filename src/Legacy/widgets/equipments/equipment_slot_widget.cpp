#include "Launcher/widgets/equipment/equipment_slot_widget.h"
#include "Launcher/dragndrop/drag_event_builder.h"
#include "Launcher/styles/items_styles.h"
#include "Content/InventroiesModule/services/equipment_service.h"
#include "Content/InventroiesModule/models/item_mime_data.h"
#include "Content/InventroiesModule/models/item.h"
#include "Content/InventroiesModule/models/container.h"

#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QPainter>
#include <QDrag>
#include <QStyle>

class EquipmentSlot::Private {
public:
  Private(EquipmentSlot* parent) : q(parent) { }
  EquipmentSlot* q;

	ItemSlotType slot;
  EquipmentWidget* parentWidget;
	EquipmentService* equipmentService;
	std::shared_ptr<ContainerItem> item;
  bool highlighted = false;
  bool dragging = false;
};

EquipmentSlot::EquipmentSlot(
	EquipmentService* equipmentService,
	ItemSlotType type,
	EquipmentWidget* parentWidget,
	QWidget* parent)
  : d(std::make_unique<Private>(this))
	, QLabel(parent) {
	setAcceptDrops(true);
	setAlignment(Qt::AlignCenter);
	setFixedSize(64, 64);

	d->equipmentService = equipmentService;
	d->slot = type;
	d->parentWidget = parentWidget;

	// Специальные размеры для некоторых слотов
	switch (type) {
	case ItemSlotType::Backpack:
	case ItemSlotType::Bag1:
	case ItemSlotType::Bag2:
		setFixedSize(48, 48);
		break;
	case ItemSlotType::WeaponLeft:
	case ItemSlotType::WeaponRight:
		setFixedSize(48, 96); // Вертикальное оружие
		break;
	case ItemSlotType::Body:
		setFixedSize(64, 96); // Тело выше
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
  return (bool)d->item;
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

ItemSlotType EquipmentSlot::slotType() const {
  return d->slot;
}

void EquipmentSlot::setItem(std::shared_ptr<ContainerItem> item) {
	const auto position = ContainerItem::convertSlotToPosition(d->slot);
	if (!d->equipmentService->canPlaceItem(item, position.y(), position.x(), false)) {
		return;
	}

	d->item = item;

	setPixmap(item.entity->icon.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
	updateVisualState();
}

void EquipmentSlot::clearItem() {
	if (d->item) {
		d->equipmentService->unequipItem(d->item, d->slot);
		d->item.reset();
		clear();
		updateVisualState();
	}
}

void EquipmentSlot::mousePressEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton) {
		startDrag();
	}

	QLabel::mousePressEvent(event);
}

void EquipmentSlot::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() == Qt::LeftButton && d->item && d->item->item->entity->container.has_value()) {
		emit containerOpened(ItemMimeData(*d->item));
	}
}

void EquipmentSlot::dragEnterEvent(QDragEnterEvent* event) {
	if (!event->mimeData()->hasFormat("application/x-game-item")) {
		event->ignore();
		return;
	}

	QByteArray data = event->mimeData()->data("application/x-game-item");
	const auto item = ItemMimeData::fromMimeData(data);
	const auto position = ContainerItem::convertSlotToPosition(d->slot);

	if (d->equipmentService->canPlaceItem(item, position.y(), position.x(), false)) {
		setHighlighted(true);
		event->acceptProposedAction();
		return;
	}

	event->ignore();
}

void EquipmentSlot::dragLeaveEvent(QDragLeaveEvent* event) {
	setHighlighted(false);
	QLabel::dragLeaveEvent(event);
}

void EquipmentSlot::startDrag() {
	if (!d->item) {
		return;
	}

	const auto itemPtr = d->item.get();

	DragEventBuilder builder(this, ItemMimeData(*itemPtr), *itemPtr->item->entity, d->equipmentService->placementId());

	const auto dropAction = builder.ExecDrag(Qt::MoveAction);

	if (dropAction == Qt::IgnoreAction) {
		QPixmap currentPixmap = (!pixmap().isNull()) ? pixmap() : QPixmap();
		setPixmap(currentPixmap);
		d->dragging = false;
		updateVisualState();
	}
	else {
		clearItem();
	}
}

void EquipmentSlot::dropEvent(QDropEvent* event) {
	setHighlighted(false);

	if (!event->mimeData()->hasFormat("application/x-game-item")) {
		event->ignore();
		return;
	}

	// в экипировку предмет может попасть только из инвентаря
	const auto inventoryIdStr = QUuid::fromString(QString::fromUtf8(event->mimeData()->data("application/x-game-item-source-inventory-id")));
	if (inventoryIdStr.isNull()) {
		event->ignore();
		return;
	}

	const auto inventoryId = inventoryIdStr;
	const auto data = event->mimeData()->data("application/x-game-item");
	const auto item = ItemMimeData::fromMimeData(data);

	if (!d->equipmentService->equipItem(item, d->slot, inventoryId)) {
		updateVisualState();
		event->ignore();
		return;
	}

	event->acceptProposedAction();
}

void EquipmentSlot::paintEvent(QPaintEvent* event) {
	QLabel::paintEvent(event);

	// Добавляем визуальные метки типа слота (опционально)
	if (!d->item.has_value() && !d->dragging) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setPen(QColor(100, 116, 139));

		switch (d->slot) {
		case ItemSlotType::Head: painter.drawText(rect(), Qt::AlignCenter, "H"); break;
		case ItemSlotType::Body: painter.drawText(rect(), Qt::AlignCenter, "B"); break;
		case ItemSlotType::WeaponLeft: painter.drawText(rect(), Qt::AlignCenter, "L"); break;
		case ItemSlotType::WeaponRight: painter.drawText(rect(), Qt::AlignCenter, "R"); break;
		case ItemSlotType::Gloves: painter.drawText(rect(), Qt::AlignCenter, "G"); break;
		case ItemSlotType::Amulet: painter.drawText(rect(), Qt::AlignCenter, "A"); break;
		case ItemSlotType::Boots: painter.drawText(rect(), Qt::AlignCenter, "F"); break;
		case ItemSlotType::Ring1: painter.drawText(rect(), Qt::AlignCenter, "R1"); break;
		case ItemSlotType::Ring2: painter.drawText(rect(), Qt::AlignCenter, "R2"); break;
		case ItemSlotType::Ring3: painter.drawText(rect(), Qt::AlignCenter, "R3"); break;
		case ItemSlotType::Backpack: painter.drawText(rect(), Qt::AlignCenter, "Bp"); break;
		case ItemSlotType::Bag1: painter.drawText(rect(), Qt::AlignCenter, "B1"); break;
		case ItemSlotType::Bag2: painter.drawText(rect(), Qt::AlignCenter, "B2"); break;
		}
	}
}

void EquipmentSlot::updateVisualState() {
	setProperty("occupied", (bool)d->item);
	setProperty("highlight", d->highlighted);

	// Применяем изменения стиля
	style()->unpolish(this);
	style()->polish(this);
	update();
}


