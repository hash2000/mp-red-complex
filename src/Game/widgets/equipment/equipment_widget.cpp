#include "Game/widgets/equipment/equipment_widget.h"
#include <QVBoxLayout>

class EquipmentWidget::Private {
public:
	Private(EquipmentWidget* parent)
	: q(parent) {

	}
	EquipmentWidget* q;
	QMap<EquipmentSlotType, EquipmentSlot*> allSlots;
};

EquipmentWidget::EquipmentWidget(QWidget* parent)
: d(std::make_unique<Private>(this)) {
	setObjectName("EquipmentWidget");
	setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	setLineWidth(1);

	setupLayout();

	// Стиль по умолчанию (можно переопределить через внешний QSS)
	setStyleSheet(R"(
        #EquipmentWidget {
            background-color: #1a202c;
            border: 1px solid #4a5568;
            border-radius: 8px;
            padding: 12px;
        }
    )");
}

EquipmentWidget::~EquipmentWidget() = default;

void EquipmentWidget::setupLayout() {
	auto* mainLayout = new QVBoxLayout(this);
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(12);

	// === Верхний ряд: оружие/щит + голова ===
	auto* topRow = new QHBoxLayout();
	topRow->setSpacing(16);
	topRow->addStretch();

	auto* leftWeaponSlot = new EquipmentSlot(EquipmentSlotType::WeaponLeft, this);
	d->allSlots[EquipmentSlotType::WeaponLeft] = leftWeaponSlot;
	topRow->addWidget(leftWeaponSlot);

	auto* headSlot = new EquipmentSlot(EquipmentSlotType::Head, this);
	d->allSlots[EquipmentSlotType::Head] = headSlot;
	topRow->addWidget(headSlot);

	auto* rightWeaponSlot = new EquipmentSlot(EquipmentSlotType::WeaponRight, this);
	d->allSlots[EquipmentSlotType::WeaponRight] = rightWeaponSlot;
	topRow->addWidget(rightWeaponSlot);

	topRow->addStretch();
	mainLayout->addLayout(topRow);

	// === Центр: тело ===
	auto* bodyRow = new QHBoxLayout();
	bodyRow->addStretch();

	auto* bodySlot = new EquipmentSlot(EquipmentSlotType::Body, this);
	d->allSlots[EquipmentSlotType::Body] = bodySlot;
	bodyRow->addWidget(bodySlot);

	bodyRow->addStretch();
	mainLayout->addLayout(bodyRow);

	// === Нижний ряд: перчатки + обувь + кольца/амулет ===
	auto* bottomRow = new QHBoxLayout();
	bottomRow->setSpacing(16);
	bottomRow->addStretch();

	// Левые перчатки
	auto* glovesLeftSlot = new EquipmentSlot(EquipmentSlotType::GlovesLeft, this);
	d->allSlots[EquipmentSlotType::GlovesLeft] = glovesLeftSlot;
	bottomRow->addWidget(glovesLeftSlot);

	// Обувь
	auto* bootsSlot = new EquipmentSlot(EquipmentSlotType::Boots, this);
	d->allSlots[EquipmentSlotType::Boots] = bootsSlot;
	bottomRow->addWidget(bootsSlot);

	// Правые перчатки
	auto* glovesRightSlot = new EquipmentSlot(EquipmentSlotType::GlovesRight, this);
	d->allSlots[EquipmentSlotType::GlovesRight] = glovesRightSlot;
	bottomRow->addWidget(glovesRightSlot);

	bottomRow->addSpacing(24);

	// Аксессуары (вертикальная колонка)
	auto* accessoriesLayout = new QVBoxLayout();
	accessoriesLayout->setSpacing(8);

	auto* amuletSlot = new EquipmentSlot(EquipmentSlotType::Amulet, this);
	d->allSlots[EquipmentSlotType::Amulet] = amuletSlot;
	accessoriesLayout->addWidget(amuletSlot);

	auto* ringLeftSlot = new EquipmentSlot(EquipmentSlotType::RingLeft, this);
	d->allSlots[EquipmentSlotType::RingLeft] = ringLeftSlot;
	accessoriesLayout->addWidget(ringLeftSlot);

	auto* ringRightSlot = new EquipmentSlot(EquipmentSlotType::RingRight, this);
	d->allSlots[EquipmentSlotType::RingRight] = ringRightSlot;
	accessoriesLayout->addWidget(ringRightSlot);

	bottomRow->addLayout(accessoriesLayout);
	bottomRow->addStretch();

	mainLayout->addLayout(bottomRow);
	mainLayout->addStretch();

	// Подключаем сигналы слотов к виджету
	for (auto* slot : d->allSlots) {
		connect(slot, &EquipmentSlot::itemEquipped, this, [this, slot](const EquipmentItem& item) {
			emit itemEquipped(item, slot->slotType());
			});
		connect(slot, &EquipmentSlot::itemRemoved, this, [this, slot](const EquipmentItem& item) {
			emit itemUnequipped(item, slot->slotType());
			});
	}
}

std::optional<EquipmentItem> EquipmentWidget::getItem(EquipmentSlotType slot) const {
	if (d->allSlots.contains(slot) && d->allSlots[slot]->isOccupied()) {
		return d->allSlots[slot]->item();
	}
	return std::nullopt;
}

bool EquipmentWidget::equipItem(const EquipmentItem& item) {
	EquipmentSlot* slot = findCompatibleSlot(item);
	if (slot && !slot->isOccupied()) {
		return slot->setItem(item);
	}
	return false;
}

void EquipmentWidget::unequipItem(EquipmentSlotType slot) {
	if (d->allSlots.contains(slot)) {
		d->allSlots[slot]->clearItem();
	}
}

void EquipmentWidget::clearAll() {
	for (auto* slot : d->allSlots) {
		slot->clearItem();
	}
}

QMap<EquipmentSlotType, EquipmentSlot*> EquipmentWidget::allSlots() const {
	return d->allSlots;
}

EquipmentSlot* EquipmentWidget::findCompatibleSlot(const EquipmentItem& item) const {
	for (auto it = d->allSlots.cbegin(); it != d->allSlots.cend(); ++it) {
		if (it.value()->canAcceptItem(item)) {
			return it.value();
		}
	}
	return nullptr;
}
