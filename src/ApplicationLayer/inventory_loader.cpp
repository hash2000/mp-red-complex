#include "ApplicationLayer/inventory_loader.h"
#include "ApplicationLayer/repositories/i_item_repository.h"
#include "ApplicationLayer/repositories/i_inventory_repository.h"
#include "ApplicationLayer/repositories/i_equipment_repository.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include "ApplicationLayer/items/items_service.h"
#include "DataLayer/inventory/inventory_item.h"
#include "DataLayer/equipment/equipment.h"
#include <QDebug>

class InventoryLoader::Private {
public:
    Private(InventoryLoader* parent)
        : q(parent) {
    }

    InventoryLoader* q;
    std::shared_ptr<IInventoryRepository> inventoryRepository;
    std::shared_ptr<IEquipmentRepository> equipmentRepository;
    std::shared_ptr<IItemRepository> itemRepository;
    ItemsService* itemsService;
};

InventoryLoader::InventoryLoader(
    std::shared_ptr<IInventoryRepository> inventoryRepository,
    std::shared_ptr<IEquipmentRepository> equipmentRepository,
    std::shared_ptr<IItemRepository> itemRepository,
    ItemsService* itemsService,
    QObject* parent)
    : d(std::make_unique<Private>(this))
    , QObject(parent) {
    d->inventoryRepository = inventoryRepository;
    d->equipmentRepository = equipmentRepository;
    d->itemRepository = itemRepository;
    d->itemsService = itemsService;
}

InventoryLoader::~InventoryLoader() = default;

std::unique_ptr<ItemPlacementService> InventoryLoader::load(const QUuid& id) {
    // Сначала пробуем загрузить как инвентарь
    auto inventory = d->inventoryRepository->findById(id);
    if (inventory) {
        auto inventoryService = std::make_unique<InventoryService>(d->itemsService);
        if (!inventoryService->load(*inventory)) {
            qWarning() << "InventoryLoader::load: failed to load inventory" << id;
            return nullptr;
        }
        return inventoryService;
    }

    // Затем пробуем загрузить как экипировку
    auto equipment = d->equipmentRepository->findById(id);
    if (equipment) {
        auto equipmentService = std::make_unique<EquipmentService>(d->itemsService);
        if (!equipmentService->load(*equipment)) {
            qWarning() << "InventoryLoader::load: failed to load equipment" << id;
            return nullptr;
        }
        return equipmentService;
    }

    return nullptr;
}

std::unique_ptr<ItemPlacementService> InventoryLoader::createInventory(const QUuid& id, const QString& name, int rows, int cols) {
    // Создаём пустой инвентарь в памяти
    Inventory inventory;
    inventory.id = id;
    inventory.name = name;
    inventory.rows = rows;
    inventory.cols = cols;
    // items пуст по умолчанию

    auto inventoryService = std::make_unique<InventoryService>(d->itemsService);
    if (!inventoryService->load(inventory)) {
        qWarning() << "InventoryLoader::createInventory: failed to initialize inventory" << id;
        return nullptr;
    }
    return inventoryService;
}

std::unique_ptr<ItemPlacementService> InventoryLoader::createEquipment(const QUuid& id) {
    // Создаём пустую экипировку в памяти
    Equipment equipment;
    equipment.id = id;
    equipment.name = QString("Equipment");
    // items пуст по умолчанию

    auto equipmentService = std::make_unique<EquipmentService>(d->itemsService);
    if (!equipmentService->load(equipment)) {
        qWarning() << "InventoryLoader::createEquipment: failed to initialize equipment" << id;
        return nullptr;
    }
    return equipmentService;
}
