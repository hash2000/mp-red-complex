#include "Game/widgets/equipment/equipment_widget.h"
#include <QVBoxLayout>

class EquipmentWidget::Private {
public:
	Private(EquipmentWidget* parent)
	: q(parent) {

	}
	EquipmentWidget* q;
	QMap<EquipmentSlot::SlotType, EquipmentSlot*> allSlots;
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

	auto* leftWeaponSlot = new EquipmentSlot(EquipmentSlot::SlotType::WeaponLeft, this);
	d->allSlots[EquipmentSlot::SlotType::WeaponLeft] = leftWeaponSlot;
	topRow->addWidget(leftWeaponSlot);

	auto* headSlot = new EquipmentSlot(EquipmentSlot::SlotType::Head, this);
	d->allSlots[EquipmentSlot::SlotType::Head] = headSlot;
	topRow->addWidget(headSlot);

	auto* rightWeaponSlot = new EquipmentSlot(EquipmentSlot::SlotType::WeaponRight, this);
	d->allSlots[EquipmentSlot::SlotType::WeaponRight] = rightWeaponSlot;
	topRow->addWidget(rightWeaponSlot);

	topRow->addStretch();
	mainLayout->addLayout(topRow);

	// === Центр: тело ===
	auto* bodyRow = new QHBoxLayout();
	bodyRow->addStretch();

	auto* bodySlot = new EquipmentSlot(EquipmentSlot::SlotType::Body, this);
	d->allSlots[EquipmentSlot::SlotType::Body] = bodySlot;
	bodyRow->addWidget(bodySlot);

	bodyRow->addStretch();
	mainLayout->addLayout(bodyRow);

	// === Нижний ряд: перчатки + обувь + кольца/амулет ===
	auto* bottomRow = new QHBoxLayout();
	bottomRow->setSpacing(16);
	bottomRow->addStretch();

	// Левые перчатки
	auto* glovesLeftSlot = new EquipmentSlot(EquipmentSlot::SlotType::GlovesLeft, this);
	d->allSlots[EquipmentSlot::SlotType::GlovesLeft] = glovesLeftSlot;
	bottomRow->addWidget(glovesLeftSlot);

	// Обувь
	auto* bootsSlot = new EquipmentSlot(EquipmentSlot::SlotType::Boots, this);
	d->allSlots[EquipmentSlot::SlotType::Boots] = bootsSlot;
	bottomRow->addWidget(bootsSlot);

	// Правые перчатки
	auto* glovesRightSlot = new EquipmentSlot(EquipmentSlot::SlotType::GlovesRight, this);
	d->allSlots[EquipmentSlot::SlotType::GlovesRight] = glovesRightSlot;
	bottomRow->addWidget(glovesRightSlot);

	bottomRow->addSpacing(24);

	// Аксессуары (вертикальная колонка)
	auto* accessoriesLayout = new QVBoxLayout();
	accessoriesLayout->setSpacing(8);

	auto* amuletSlot = new EquipmentSlot(EquipmentSlot::SlotType::Amulet, this);
	d->allSlots[EquipmentSlot::SlotType::Amulet] = amuletSlot;
	accessoriesLayout->addWidget(amuletSlot);

	auto* ringLeftSlot = new EquipmentSlot(EquipmentSlot::SlotType::RingLeft, this);
	d->allSlots[EquipmentSlot::SlotType::RingLeft] = ringLeftSlot;
	accessoriesLayout->addWidget(ringLeftSlot);

	auto* ringRightSlot = new EquipmentSlot(EquipmentSlot::SlotType::RingRight, this);
	d->allSlots[EquipmentSlot::SlotType::RingRight] = ringRightSlot;
	accessoriesLayout->addWidget(ringRightSlot);

	bottomRow->addLayout(accessoriesLayout);
	bottomRow->addStretch();

	mainLayout->addLayout(bottomRow);
	mainLayout->addStretch();

	// Подключаем сигналы слотов к виджету
	for (auto* slot : d->allSlots) {
		connect(slot, &EquipmentSlot::itemEquipped, this, [this, slot](const Item& item) {
			emit itemEquipped(item, slot->slotType());
			});
		connect(slot, &EquipmentSlot::itemRemoved, this, [this, slot](const Item& item) {
			emit itemUnequipped(item, slot->slotType());
			});
	}
}

std::optional<Item> EquipmentWidget::getItem(EquipmentSlot::SlotType slot) const {
	if (d->allSlots.contains(slot) && d->allSlots[slot]->isOccupied()) {
		return d->allSlots[slot]->item();
	}
	return std::nullopt;
}

bool EquipmentWidget::equipItem(const Item& item) {
	EquipmentSlot* slot = findCompatibleSlot(item);
	if (slot && !slot->isOccupied()) {
		return slot->setItem(item);
	}
	return false;
}

void EquipmentWidget::unequipItem(EquipmentSlot::SlotType slot) {
	if (d->allSlots.contains(slot)) {
		d->allSlots[slot]->clearItem();
	}
}

void EquipmentWidget::clearAll() {
	for (auto* slot : d->allSlots) {
		slot->clearItem();
	}
}

QMap<EquipmentSlot::SlotType, EquipmentSlot*> EquipmentWidget::allSlots() const {
	return d->allSlots;
}

EquipmentSlot* EquipmentWidget::findCompatibleSlot(const Item& item) const {
	for (auto it = d->allSlots.cbegin(); it != d->allSlots.cend(); ++it) {
		if (it.value()->canAcceptItem(item)) {
			return it.value();
		}
	}
	return nullptr;
}
