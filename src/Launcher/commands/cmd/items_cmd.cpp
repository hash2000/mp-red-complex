#include "Launcher/commands/cmd/items_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/command_console/console_table.h"
#include "Launcher/commands/command_console/console_image.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
#include "Content/InventoriesModule/services/items_service.h"
#include "Content/InventoriesModule/models/item.h"
#include "Content/InventoriesModule/models/extensions/entity_extensions.h"

#include <QMdiSubWindow>
#include <QUuid>

class ItemsCommand::Private {
public:
	Private(ItemsCommand* parent) : q(parent) {}
	ItemsCommand* q;

	void printEntity(ConsoleTable& table, std::shared_ptr<ItemEntity> entity);
	void printItem(ConsoleTable& table, std::shared_ptr<Item> item);
	bool showEntities(CommandContext* context);
	bool showContainerItems(CommandContext* context, const QUuid& containerId);
	bool showItem(CommandContext* context, const QUuid& itemId);
	bool showContainers(CommandContext* context);
	bool showEquipments(CommandContext* context);
	bool changeContainerId(CommandContext* context, const QUuid& itemId, const QUuid& containerId);
};

ItemsCommand::ItemsCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, CommandAbstraction(parent) {
}

ItemsCommand::~ItemsCommand() = default;

QString ItemsCommand::help() const {
	return R"(items action:
	show-entities
	show-container id:{containerId}
	show-item id:{itemId}
	show-containers
	show-equipments
	change-container itemId:{itemId} containerId:{containerId}
	)";
}

void ItemsCommand::Private::printEntity(ConsoleTable& table, std::shared_ptr<ItemEntity> entity) {
	using namespace Inventory::Entities;
	table.addRow({
		entity->icon,
		entity->id,
		entity->name,
		entity->description,
		itemTypeToString(entity->type),
		itemSubTypeToString(entity->subType),
		entity->width,
		entity->height,
		itemRarityTypeToString(entity->rarity),
		entity->container.has_value() ? entity->container->rows : 1,
		entity->container.has_value() ? entity->container->cols : 1,
		entity->maxStack,
		});
}

void ItemsCommand::Private::printItem(ConsoleTable& table, std::shared_ptr<Item> item) {
	table.addRow({
		item->entity->icon,
		item->id.toString(QUuid::WithoutBraces),
		item->entity->id,
		item->containerId.toString(QUuid::WithoutBraces),
		item->level,
		item->count,
		item->position.base,
		item->position.secondary,
		});
}

bool ItemsCommand::Private::showEntities(CommandContext* context) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto itemsService = services->itemsService();
	auto entities = itemsService->entities();

	ConsoleTable table({ "..", "Id", "Name", "Desc", "Type", "SubType", "w", "h", "rarity", "rows", "cols", "stack" });

	for (auto item : entities) {
		printEntity(table, item);
	}

	context->print(table);
	return true;
}

bool ItemsCommand::Private::showContainerItems(CommandContext* context, const QUuid& containerId) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto itemsService = services->itemsService();
	auto items = itemsService->itemsFromContainer(containerId);
	if (items.empty()) {
		context->printWarning(QString("Container is empty or undefined. %1")
			.arg(containerId
				.toString(QUuid::WithoutBraces)));
		return false;
	}

	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	for (auto item : items) {
		printItem(table, item);
	}

	context->print(table);
	return true;
}

bool ItemsCommand::Private::showItem(CommandContext* context, const QUuid& itemId) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto itemsService = services->itemsService();
	auto item = itemsService->item(itemId);
	if (!item) {
		context->printWarning(QString("Item is undefined. %1")
			.arg(itemId
				.toString(QUuid::WithoutBraces)));
		return false;
	}

	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	printItem(table, item);
	context->print(table);
	return true;
}

bool ItemsCommand::Private::showContainers(CommandContext* context) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto itemsService = services->itemsService();
	auto items = itemsService->containers();

	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	for (auto item : items) {
		printItem(table, item);
	}

	context->print(table);
	return true;
}

bool ItemsCommand::Private::showEquipments(CommandContext* context) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto itemsService = services->itemsService();
	auto items = itemsService->equipments();

	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	for (auto item : items) {
		printItem(table, item);
	}

	context->print(table);
	return true;
}

bool ItemsCommand::Private::changeContainerId(CommandContext* context, const QUuid& itemId, const QUuid& containerId) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto itemsService = services->itemsService();
	auto item = itemsService->item(itemId);
	if (!item) {
		context->printWarning(QString("Item is undefined. %1")
			.arg(itemId
			.toString(QUuid::WithoutBraces)));
		return false;
	}

	itemsService->changeItemContainer(item, containerId);
	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	printItem(table, item);
	context->print(table);
	return true;
}

bool ItemsCommand::execute(CommandContext* context, const QStringList& args) {
	const auto action = parseArgsValue(args, "action");
	if (action.isEmpty()) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "show-container") return d->showContainerItems(context,
		QUuid::fromString(parseArgsValue(args, "id")));
	else if (action == "show-item") return d->showItem(context,
		QUuid::fromString(parseArgsValue(args, "id")));
	else if (action == "change-container") return d->changeContainerId(context,
		QUuid::fromString(parseArgsValue(args, "itemId")),
		QUuid::fromString(parseArgsValue(args, "containerId")));
	else if (action == "show-containers") return d->showContainers(context);
	else if (action == "show-equipments") return d->showEquipments(context);
	else if (action == "show-entities") return d->showEntities(context);

	return false;
}
