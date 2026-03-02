#include "Game/widgets/equipment/equipment_widget.h"
#include "Game/widgets/equipment/equipment_slot_widget.h"
#include "ApplicationLayer/equipment/equipments_service.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include <QVBoxLayout>

class EquipmentWidget::Private {
public:
	Private(EquipmentWidget* parent)
	: q(parent) {

	}

	void setupLayout(EquipmentService* equipmentService) {
		auto mainLayout = new QVBoxLayout(q);
		mainLayout->setContentsMargins(10, 10, 10, 10);
		mainLayout->setSpacing(12);

		// === Верхний ряд: оружие/щит + голова ===
		auto topRow = new QHBoxLayout();
		topRow->setSpacing(16);
		topRow->addStretch();

		auto leftWeaponSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::WeaponLeft, q);
		allSlots[EquipmentSlotType::WeaponLeft] = leftWeaponSlot;
		topRow->addWidget(leftWeaponSlot);

		auto headSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::Head, q);
		allSlots[EquipmentSlotType::Head] = headSlot;
		topRow->addWidget(headSlot);

		auto rightWeaponSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::WeaponRight, q);
		allSlots[EquipmentSlotType::WeaponRight] = rightWeaponSlot;
		topRow->addWidget(rightWeaponSlot);

		topRow->addStretch();
		mainLayout->addLayout(topRow);

		// === Центр: тело ===
		auto bodyRow = new QHBoxLayout();
		bodyRow->addStretch();

		// Рюкзак
		auto backpack = new EquipmentSlot(equipmentService, EquipmentSlotType::Backpack, q);
		allSlots[EquipmentSlotType::Backpack] = backpack;
		bodyRow->addWidget(backpack);

		auto bodySlot = new EquipmentSlot(equipmentService, EquipmentSlotType::Body, q);
		allSlots[EquipmentSlotType::Body] = bodySlot;
		bodyRow->addWidget(bodySlot);

		// колонка для подсумков
		auto bagCol = new QVBoxLayout();

		auto bag1Slot = new EquipmentSlot(equipmentService, EquipmentSlotType::Bag1, q);
		allSlots[EquipmentSlotType::Bag1] = bag1Slot;
		bagCol->addWidget(bag1Slot);

		auto bag2Slot = new EquipmentSlot(equipmentService, EquipmentSlotType::Bag2, q);
		allSlots[EquipmentSlotType::Bag2] = bag2Slot;
		bagCol->addWidget(bag2Slot);

		bodyRow->addLayout(bagCol);

		bodyRow->addStretch();
		mainLayout->addLayout(bodyRow);

		// === Нижний ряд: перчатки + обувь + кольца/амулет ===
		auto bottomRow = new QHBoxLayout();
		bottomRow->setSpacing(16);
		bottomRow->addStretch();

		// Левые перчатки
		auto glovesLeftSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::GlovesLeft, q);
		allSlots[EquipmentSlotType::GlovesLeft] = glovesLeftSlot;
		bottomRow->addWidget(glovesLeftSlot);

		// Обувь
		auto bootsSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::Boots, q);
		allSlots[EquipmentSlotType::Boots] = bootsSlot;
		bottomRow->addWidget(bootsSlot);

		// Правые перчатки
		auto glovesRightSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::GlovesRight, q);
		allSlots[EquipmentSlotType::GlovesRight] = glovesRightSlot;
		bottomRow->addWidget(glovesRightSlot);

		bottomRow->addSpacing(24);

		// Аксессуары (вертикальная колонка)
		auto accessoriesLayout = new QVBoxLayout();
		accessoriesLayout->setSpacing(8);

		auto amuletSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::Amulet, q);
		allSlots[EquipmentSlotType::Amulet] = amuletSlot;
		accessoriesLayout->addWidget(amuletSlot);

		auto ringLeftSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::RingLeft, q);
		allSlots[EquipmentSlotType::RingLeft] = ringLeftSlot;
		accessoriesLayout->addWidget(ringLeftSlot);

		auto ringRightSlot = new EquipmentSlot(equipmentService, EquipmentSlotType::RingRight, q);
		allSlots[EquipmentSlotType::RingRight] = ringRightSlot;
		accessoriesLayout->addWidget(ringRightSlot);

		bottomRow->addLayout(accessoriesLayout);
		bottomRow->addStretch();

		mainLayout->addLayout(bottomRow);
		mainLayout->addStretch();
	}

	EquipmentSlot* findCompatibleSlot(const EquipmentItem& item) const {
		for (auto it = allSlots.cbegin(); it != allSlots.cend(); ++it) {
			//if (it.value()->canAcceptItem(item)) {
			//	return it.value();
			//}
		}
		return nullptr;
	}

	EquipmentWidget* q;
	QMap<EquipmentSlotType, EquipmentSlot*> allSlots;
	EquipmentsService* equipmentsService = nullptr;
	EquipmentService* equipmentService = nullptr;
};

EquipmentWidget::EquipmentWidget(EquipmentsService* equipmentsService, QWidget* parent)
: d(std::make_unique<Private>(this)) {
	d->equipmentsService = equipmentsService;

	setObjectName("EquipmentWidget");
	setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	setLineWidth(1);

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

bool EquipmentWidget::setInventoryService(const QUuid& id) {
	auto equipmentService = d->equipmentsService->equipment(id, true);
	if (!equipmentService) {
		return false;
	}

	if (d->equipmentService) {
		disconnect(d->equipmentService, nullptr, this, nullptr);

		for (auto widget : d->allSlots) {
			widget->clearItem();
		}
	}
	else {
		d->setupLayout(equipmentService);
	}

	d->equipmentService = equipmentService;

	// Подключаем сигналы слотов к виджету
	for (auto slot : d->allSlots) {
		connect(slot, &EquipmentSlot::itemEquipped, this, [this, slot](const EquipmentItem& item) {
			emit itemEquipped(item, slot->slotType());
			});
		connect(slot, &EquipmentSlot::itemRemoved, this, [this, slot](const EquipmentItem& item) {
			emit itemUnequipped(item, slot->slotType());
			});
	}

	return true;
}

std::optional<EquipmentItem> EquipmentWidget::getItem(EquipmentSlotType slot) const {
	if (d->allSlots.contains(slot) && d->allSlots[slot]->isOccupied()) {
		return d->allSlots[slot]->item();
	}
	return std::nullopt;
}

bool EquipmentWidget::equipItem(const EquipmentItem& item) {
	EquipmentSlot* slot = d->findCompatibleSlot(item);
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
	for (auto slot : d->allSlots) {
		slot->clearItem();
	}
}

QMap<EquipmentSlotType, EquipmentSlot*> EquipmentWidget::allSlots() const {
	return d->allSlots;
}


