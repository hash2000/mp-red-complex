#include "ApplicationLayer/inventory/inventory_save_handler.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventory/inventory_item_handler.h"
#include "DataLayer/inventory/i_inventory_data_writer.h"
#include "DataLayer/inventory/inventory_item.h"
#include <QDebug>

class InventorySaveHandler::Private {
public:
	Private(InventorySaveHandler* parent)
		: q(parent) {
	}

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

	bool save() {
		if (!inventoryService || !dataWriter) {
			return false;
		}

		auto inventoryData = toInventoryData(inventoryService);
		return dataWriter->saveInventory(inventoryId, inventoryData);
	}

	InventorySaveHandler* q;
	InventoryService* inventoryService = nullptr;
	IInventoryDataWriter* dataWriter = nullptr;
	QUuid inventoryId;
};

InventorySaveHandler::InventorySaveHandler(
	InventoryService* inventoryService,
	IInventoryDataWriter* dataWriter,
	const QUuid& inventoryId,
	QObject* parent)
: QObject(parent)
, d(std::make_unique<Private>(this)) {
	d->inventoryService = inventoryService;
	d->dataWriter = dataWriter;
	d->inventoryId = inventoryId;

	// Подключаемся ко всем сигналам изменений
	connect(inventoryService, &InventoryService::placeItemEvent,
		this, &InventorySaveHandler::onItemChanged);
	connect(inventoryService, &InventoryService::removeItemEvent,
		this, &InventorySaveHandler::onItemChanged);
	connect(inventoryService, &InventoryService::moveItemEvent,
		this, &InventorySaveHandler::onItemChanged);
	connect(inventoryService, &InventoryService::itemCountChanged,
		this, &InventorySaveHandler::onItemChanged);
}

InventorySaveHandler::~InventorySaveHandler() = default;

void InventorySaveHandler::onItemChanged() {
	if (d->save()) {
		qDebug() << "Inventory saved:" << d->inventoryId;
	}
	else {
		qWarning() << "Failed to save inventory:" << d->inventoryId;
	}
}
