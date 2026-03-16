#include "ApplicationLayer/inventories_save_manager.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include "DataLayer/inventory/inventory_data_writer.h"
#include "DataLayer/equipment/equipment_data_writer.h"
#include "DataLayer/inventory/inventory_item.h"
#include "DataLayer/equipment/equipment.h"
#include <QDebug>

class InventoriesSaveManager::Private {
public:
	Private(InventoriesSaveManager* parent)
		: q(parent) {
	}

	InventoriesSaveManager* q;
	InventoriesService* inventoriesService = nullptr;
	InventoryDataWriter* inventoryDataWriter = nullptr;
	EquipmentDataWriter* equipmentDataWriter = nullptr;

	Inventory toInventoryData(const InventoryService* service) {
		Inventory inventory;
		inventory.id = service->placementId();
		inventory.name = service->placementName();
		inventory.rows = service->rows();
		inventory.cols = service->cols();

		for (const auto& item : service->items()) {
			InventoryItem invItem;
			invItem.id = item.id;
			invItem.x = item.x;
			invItem.y = item.y;
			invItem.count = item.count;
			inventory.items.push_back(invItem);
		}

		return inventory;
	}

	Equipment toEquipmentData(const EquipmentService* service) {
		Equipment equipment;
		equipment.id = service->placementId();

		for (const auto& [slot, item] : service->items()) {
			if (!item) {
				continue;
			}

			EquipmentItem eqItem;
			eqItem.id = item->id;
			eqItem.slot = item->slot;
			equipment.items.push_back(eqItem);
		}

		return equipment;
	}

	bool saveAll() {
		if (!inventoriesService) {
			return false;
		}

		bool success = true;
		const auto ids = inventoriesService->loadedPlacementIds();

		for (const auto& id : ids) {
			auto placementService = inventoriesService->placementService(id, false);

			// Пробуем получить как инвентарь
			auto inventoryService = dynamic_cast<InventoryService*>(placementService);
			if (inventoryService && inventoryDataWriter) {
				auto inventoryData = toInventoryData(inventoryService);
				if (!inventoryDataWriter->saveInventory(id, inventoryData)) {
					qWarning() << "InventoriesSaveManager: failed to save inventory" << id;
					success = false;
				}
				else {
					qDebug() << "InventoriesSaveManager: saved inventory" << id;
				}
				continue;
			}

			// Пробуем получить как экипировку
			auto equipmentService = dynamic_cast<EquipmentService*>(placementService);
			if (equipmentService && equipmentDataWriter) {
				auto equipmentData = toEquipmentData(equipmentService);
				if (!equipmentDataWriter->saveEquipment(id, equipmentData)) {
					qWarning() << "InventoriesSaveManager: failed to save equipment" << id;
					success = false;
				}
				else {
					qDebug() << "InventoriesSaveManager: saved equipment" << id;
				}
				continue;
			}
		}

		return success;
	}
};

InventoriesSaveManager::InventoriesSaveManager(
	InventoriesService* inventoriesService,
	InventoryDataWriter* inventoryDataWriter,
	EquipmentDataWriter* equipmentDataWriter,
	QObject* parent)
: QObject(parent)
, d(std::make_unique<Private>(this)) {
	d->inventoriesService = inventoriesService;
	d->inventoryDataWriter = inventoryDataWriter;
	d->equipmentDataWriter = equipmentDataWriter;
}

InventoriesSaveManager::~InventoriesSaveManager() = default;

void InventoriesSaveManager::saveAll() {
	d->saveAll();
}
