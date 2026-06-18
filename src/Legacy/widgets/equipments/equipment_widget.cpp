#include "Launcher/widgets/equipment/equipment_widget.h"
#include "Launcher/widgets/equipment/equipment_slot_widget.h"
#include "Content/InventroiesModule/services/equipment_service.h"
#include "Content/InventroiesModule/services/inventory_service.h"
#include "Content/InventroiesModule/services/inventories_service.h"
#include "Content/InventroiesModule/models/item_mime_data.h"
#include "Content/InventroiesModule/models/item.h"
#include "Content/InventroiesModule/models/container.h"
#include <QVBoxLayout>
#include <QUuid>
#include <map>

class EquipmentWidget::Private {
public:
	Private(EquipmentWidget* parent) : q(parent) { }
	EquipmentWidget* q;

	std::map<ItemSlotType, EquipmentSlot*> allSlots;
	EquipmentService* equipmentService = nullptr;
	InventoriesService* inventoriesService = nullptr;

	void setupLayout();
	void populateSlots();
	void connectSlots();
};

EquipmentWidget::EquipmentWidget(InventoriesService* inventoriesService, QWidget* parent)
: d(std::make_unique<Private>(this)) {
	d->inventoriesService = inventoriesService;

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


void EquipmentWidget::Private::setupLayout() {
	auto mainLayout = new QVBoxLayout(q);
	mainLayout->setContentsMargins(10, 10, 10, 10);
	mainLayout->setSpacing(12);

	// === Верхний ряд: оружие/щит + голова ===
	auto topRow = new QHBoxLayout();
	topRow->setSpacing(16);
	topRow->addStretch();

	auto leftWeaponSlot = new EquipmentSlot(equipmentService, ItemSlotType::WeaponLeft, q);
	allSlots[ItemSlotType::WeaponLeft] = leftWeaponSlot;
	topRow->addWidget(leftWeaponSlot);

	auto headSlot = new EquipmentSlot(equipmentService, ItemSlotType::Head, q);
	allSlots[ItemSlotType::Head] = headSlot;
	topRow->addWidget(headSlot);

	auto rightWeaponSlot = new EquipmentSlot(equipmentService, ItemSlotType::WeaponRight, q);
	allSlots[ItemSlotType::WeaponRight] = rightWeaponSlot;
	topRow->addWidget(rightWeaponSlot);

	topRow->addStretch();
	mainLayout->addLayout(topRow);

	// === Центр: тело ===
	auto bodyRow = new QHBoxLayout();
	bodyRow->addStretch();

	// Рюкзак
	auto backpack = new EquipmentSlot(equipmentService, ItemSlotType::Backpack, q);
	allSlots[ItemSlotType::Backpack] = backpack;
	bodyRow->addWidget(backpack);

	auto bodySlot = new EquipmentSlot(equipmentService, ItemSlotType::Body, q);
	allSlots[ItemSlotType::Body] = bodySlot;
	bodyRow->addWidget(bodySlot);

	// колонка для подсумков
	auto bagCol = new QVBoxLayout();

	auto bag1Slot = new EquipmentSlot(equipmentService, ItemSlotType::Bag1, q);
	allSlots[ItemSlotType::Bag1] = bag1Slot;
	bagCol->addWidget(bag1Slot);

	auto bag2Slot = new EquipmentSlot(equipmentService, ItemSlotType::Bag2, q);
	allSlots[ItemSlotType::Bag2] = bag2Slot;
	bagCol->addWidget(bag2Slot);

	bodyRow->addLayout(bagCol);

	bodyRow->addStretch();
	mainLayout->addLayout(bodyRow);

	// === Нижний ряд: перчатки + обувь + кольца/амулет ===
	auto bottomRow = new QHBoxLayout();
	bottomRow->setSpacing(16);
	bottomRow->addStretch();

	// Перчатки
	auto glovesLeftSlot = new EquipmentSlot(equipmentService, ItemSlotType::Gloves, q);
	allSlots[ItemSlotType::Gloves] = glovesLeftSlot;
	bottomRow->addWidget(glovesLeftSlot);

	// Амулет
	auto bootsSlot = new EquipmentSlot(equipmentService, ItemSlotType::Amulet, q);
	allSlots[ItemSlotType::Amulet] = bootsSlot;
	bottomRow->addWidget(bootsSlot);

	// Обувь
	auto glovesRightSlot = new EquipmentSlot(equipmentService, ItemSlotType::Boots, q);
	allSlots[ItemSlotType::Boots] = glovesRightSlot;
	bottomRow->addWidget(glovesRightSlot);

	bottomRow->addSpacing(24);

	// Аксессуары (вертикальная колонка)
	auto accessoriesLayout = new QVBoxLayout();
	accessoriesLayout->setSpacing(8);

	auto amuletSlot = new EquipmentSlot(equipmentService, ItemSlotType::Ring1, q);
	allSlots[ItemSlotType::Ring1] = amuletSlot;
	accessoriesLayout->addWidget(amuletSlot);

	auto ringLeftSlot = new EquipmentSlot(equipmentService, ItemSlotType::Ring2, q);
	allSlots[ItemSlotType::Ring2] = ringLeftSlot;
	accessoriesLayout->addWidget(ringLeftSlot);

	auto ringRightSlot = new EquipmentSlot(equipmentService, ItemSlotType::Ring3, q);
	allSlots[ItemSlotType::Ring3] = ringRightSlot;
	accessoriesLayout->addWidget(ringRightSlot);

	bottomRow->addLayout(accessoriesLayout);
	bottomRow->addStretch();

	mainLayout->addLayout(bottomRow);
	mainLayout->addStretch();
}

void EquipmentWidget::Private::populateSlots() {
	// Инициализируем слоты предметами из загруженной экипировки
	for (const auto& [slot, item] : equipmentService->items()) {
		const auto it = allSlots.find(slot);
		if (it != allSlots.end() && item) {
			it->second->setItem(*item);
		}
	}
}

void EquipmentWidget::Private::connectSlots() {
	for (const auto& slot : allSlots) {
		connect(slot.second, &EquipmentSlot::containerOpened, q, &EquipmentWidget::containerOpened);
	}
}

bool EquipmentWidget::setEquipmentService(const QUuid& id) {
	auto equipmentService = static_cast<EquipmentService*>(d->inventoriesService->placementService(id, true));
	if (!equipmentService) {
		return false;
	}

	if (d->equipmentService) {
		disconnect(d->equipmentService, nullptr, this, nullptr);

		for (auto widget : d->allSlots) {
			widget.second->clearItem();
		}
	}

	d->equipmentService = equipmentService;
	d->setupLayout();
	d->populateSlots();
	d->connectSlots();

	connect(d->equipmentService, &EquipmentService::itemEquipped, this, &EquipmentWidget::onItemEquipped);
	connect(d->equipmentService, &EquipmentService::itemUnequipped, this, &EquipmentWidget::onItemUnequipped);

	return true;
}

void EquipmentWidget::onItemEquipped(const ContainerItem& item, ItemSlotType slot, const QUuid& inventoryId) {
	const auto &it = d->allSlots.find(slot);
	if (it == d->allSlots.end()) {
		return;
	}

	auto inventory = d->inventoriesService->placementService(inventoryId, false);
	if (inventory) {
		inventory->removeItem(ItemMimeData(item));
	}

	it->second->setItem(item);
}

void EquipmentWidget::onItemUnequipped(const ContainerItem& item, ItemSlotType slot) {

}

