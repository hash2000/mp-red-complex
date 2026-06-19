#include "Launcher/commands/cmd/items_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
//#include "Content/InventroiesModule/services/inventory_service.h"
//#include "Content/InventroiesModule/services/inventories_service.h"
#include "Content/InventroiesModule/services/items_service.h"
#include "Content/InventroiesModule/models/item_mime_data.h"

#include <QMdiSubWindow>
#include <QUuid>

class ItemsCommand::Private {
public:
	Private(ItemsCommand* parent) : q(parent) {}
	ItemsCommand* q;

	void printItem(CommandContext* context, std::shared_ptr<Item> item);
	bool showContainerItems(CommandContext* context, const QUuid& containerId);
	bool showItem(CommandContext* context, const QUuid& containerId);
};

ItemsCommand::ItemsCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, CommandAbstraction(parent) {
}

ItemsCommand::~ItemsCommand() = default;

void ItemsCommand::Private::printItem(CommandContext* context, std::shared_ptr<Item> item) {

}

bool ItemsCommand::Private::showContainerItems(CommandContext* context, const QUuid& containerId) {


	return false;
}

bool ItemsCommand::Private::showItem(CommandContext* context, const QUuid& containerId) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto itemsService = services->itemsService();
	auto item = itemsService->item(containerId);
	if (!item) {
		return false;
	}

	printItem(context, item);
	return true;
}

bool ItemsCommand::execute(CommandContext* context, const QStringList& args) {
	const auto action = parseArgsValue(args, "action");
	if (action.isEmpty()) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "show-container") return d->showContainerItems(context, QUuid::fromString(parseArgsValue(args, "id")));
	if (action == "show-item") return d->showContainerItems(context, QUuid::fromString(parseArgsValue(args, "id")));




	//auto controller = context->controllers()->windowsController();
	//auto services = context->services();
	//auto inventoriesService = services->inventoriesService();
	//auto itemsService = services->itemsService();

	//if (args.count() < 3) {
	//	context->printError(QString("Usage: %1").arg(help()));
	//	return false;
	//}

	//bool ok;

	//const auto entityId = args.filter(QRegularExpression("^entity:")).value(0).mid(7);
	//const auto count = args.filter(QRegularExpression("^count:")).value(0).mid(6).toInt(&ok);
	//if (!ok) {
	//	context->printError(QString("Usage: %1").arg(help()));
	//	return false;
	//}

	//const auto inventoryId = args.filter(QRegularExpression("^inventory:")).value(0).mid(10);

	//// Получаем сервис инвентаря
	//auto inventoryService = static_cast<InventoryService*>(
	//	inventoriesService->placementService(QUuid::fromString(inventoryId), false));

	//if (!inventoryService) {
	//	context->printError(QString("Failed to get inventory service for: %1")
	//		.arg(inventoryId));
	//	return false;
	//}

	//// Создаём ItemMimeData для нового предмета
	//const auto newItem = itemsService->createItemByEntity(entityId);
	//if (!newItem) {
	//	qWarning() << "Failed to create item:" << entityId;
	//	return false;
	//}

	//ItemMimeData mimeData(*newItem);
	//mimeData.count = count;

	//// Пытаемся найти свободное место в инвентаре
	//const auto freeSpace = inventoryService->findFreeSpace(mimeData, true);
	//if (!freeSpace.has_value()) {
	//	qWarning() << "No free space in inventory for item:" << entityId;
	//	return false;
	//}

	//// Размещаем предмет в инвентаре
	//mimeData.x = freeSpace->x();
	//mimeData.y = freeSpace->y();

	//if (inventoryService->placeItem(mimeData)) {
	//	context->printSuccess(QString("Item placed in inventory: %1 at %2, %3")
	//		.arg(newItem->id.toString())
	//		.arg(QString::number(mimeData.x))
	//		.arg(QString::number(mimeData.y)));
	//}
	//else {
	//	qWarning() << "Failed to place item in inventory:" << newItem->id;
	//	return false;
	//}

	//return true;

	return false;
}
