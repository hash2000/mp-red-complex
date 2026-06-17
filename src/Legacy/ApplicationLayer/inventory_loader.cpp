#include "ApplicationLayer/inventory_loader.h"
#include "ApplicationLayer/items/i_item_repository.h"
#include "ApplicationLayer/inventory/i_inventory_repository.h"
#include "ApplicationLayer/equipment/i_equipment_repository.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include "ApplicationLayer/items/items_service.h"
#include "DataLayer/inventory/inventory_item.h"
#include "DataLayer/equipment/equipment.h"
#include "DataLayer/items/item.h"
#include <QDebug>

class InventoryLoader::Private {
public:
    Private(InventoryLoader* parent)
        : q(parent) {
    }

		void loadPermissions(IItemPlacementService* service, const QUuid& id) {
			const auto itemPtr = itemsService->itemById(id);
			if (!itemPtr || !itemPtr->entity || !itemPtr->entity->container.has_value()) {
				return;
			}

			service->addResourcesPermissions(
				itemPtr->entity->container->permissions.resources.all,
				itemPtr->entity->container->permissions.resources.any);
		}

    InventoryLoader* q;
    IInventoryRepository* inventoryRepository;
    IEquipmentRepository* equipmentRepository;
    IItemRepository* itemRepository;
    ItemsService* itemsService;
};

InventoryLoader::InventoryLoader(
    IInventoryRepository* inventoryRepository,
    IEquipmentRepository* equipmentRepository,
    IItemRepository* itemRepository,
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

IItemPlacementService* InventoryLoader::load(const QUuid& id) {
    // Сначала пробуем загрузить как инвентарь
    auto inventory = d->inventoryRepository->findById(id);
    if (inventory) {
        auto inventoryService = new InventoryService(d->itemsService);
        if (!inventoryService->load(*inventory)) {
            qWarning() << "InventoryLoader::load: failed to load inventory" << id;
						delete inventoryService;
            return nullptr;
        }

				d->loadPermissions(inventoryService, id);
        return inventoryService;
    }

    // Затем пробуем загрузить как экипировку
    auto equipment = d->equipmentRepository->findById(id);
    if (equipment) {
        auto equipmentService = new EquipmentService(d->itemsService);
        if (!equipmentService->load(*equipment)) {
            qWarning() << "InventoryLoader::load: failed to load equipment" << id;
						delete equipmentService;
            return nullptr;
        }

				d->loadPermissions(equipmentService, id);
        return equipmentService;
    }

    return nullptr;
}

IItemPlacementService* InventoryLoader::createInventory(const QUuid& id, const QString& name, const ItemContainer& container) {
    // Создаём пустой инвентарь в памяти
    Inventory inventory;
    inventory.id = id;
    inventory.name = name;
    inventory.rows = container.rows;
    inventory.cols = container.cols;
    // items пуст по умолчанию

    auto inventoryService = new InventoryService(d->itemsService);
    if (!inventoryService->load(inventory)) {
        qWarning() << "InventoryLoader::createInventory: failed to initialize inventory" << id;
        return nullptr;
    }

		d->loadPermissions(inventoryService, id);
    return inventoryService;
}

IItemPlacementService* InventoryLoader::createEquipment(const QUuid& id) {
    // Создаём пустую экипировку в памяти
    Equipment equipment;
    equipment.id = id;
    equipment.name = QString("Equipment");
    // items пуст по умолчанию

    auto equipmentService = new EquipmentService(d->itemsService);
    if (!equipmentService->load(equipment)) {
        qWarning() << "InventoryLoader::createEquipment: failed to initialize equipment" << id;
        return nullptr;
    }

		d->loadPermissions(equipmentService, id);
    return equipmentService;
}
