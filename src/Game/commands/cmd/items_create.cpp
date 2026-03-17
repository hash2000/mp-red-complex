#include "Game/commands/cmd/items_create.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/controllers/windows_controller.h"
#include "Game/app_controller.h"
#include "Game/controllers.h"
#include "Game/services.h"
#include "Game/windows_builder.h"
#include "ApplicationLayer/inventory/inventory_service.h"
#include "ApplicationLayer/inventories_service.h"
#include "ApplicationLayer/items/items_service.h"
#include "ApplicationLayer/items/item_mime_data.h"
#include <QMdiSubWindow>
#include <QUuid>

ItemsCreateCommand::~ItemsCreateCommand() = default;

bool ItemsCreateCommand::execute(CommandContext* context, const QStringList& args) {
	auto app = context->applicationController();
	auto controller = app->controllers()->windowsController();
	auto services = app->services();
	auto inventoriesService = services->inventoriesService();
	auto itemsService = services->itemsService();

	if (args.count() < 3) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	bool ok;

	const auto entityId = args.at(0);
	const auto count = args.at(1).toInt(&ok);
	if (!ok) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	const auto inventoryId = args.at(2);

	// Получаем сервис инвентаря
	auto inventoryService = static_cast<InventoryService*>(
		inventoriesService->placementService(QUuid::fromString(inventoryId), false));

	if (!inventoryService) {
		context->printError(QString("Failed to get inventory service for: %1")
			.arg(inventoryId));
		return false;
	}

	// Создаём ItemMimeData для нового предмета
	const auto newItem = itemsService->createItemByEntity(entityId);
	if (!newItem) {
		qWarning() << "Failed to create item:" << entityId;
		return false;
	}

	ItemMimeData mimeData(*newItem);
	mimeData.count = count;

	// Пытаемся найти свободное место в инвентаре
	const auto freeSpace = inventoryService->findFreeSpace(mimeData, true);
	if (!freeSpace.has_value()) {
		qWarning() << "No free space in inventory for item:" << entityId;
		return false;
	}

	// Размещаем предмет в инвентаре
	mimeData.x = freeSpace->x();
	mimeData.y = freeSpace->y();

	if (inventoryService->placeItem(mimeData)) {
		context->printSuccess(QString("Item placed in inventory: %1 at %2, %3")
			.arg(newItem->id.toString())
			.arg(QString::number(mimeData.x))
			.arg(QString::number(mimeData.y)));
	}
	else {
		qWarning() << "Failed to place item in inventory:" << newItem->id;
		return false;
	}

	return true;
}
