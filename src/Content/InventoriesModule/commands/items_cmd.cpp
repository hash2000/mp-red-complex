#include "Content/InventoriesModule/commands/items_cmd.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_image.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Engine/services/services_registry.h"
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
	bool showEntities(std::shared_ptr<CommandContext> context);
	bool showContainerItems(std::shared_ptr<CommandContext> context, const QUuid& containerId);
	bool showItem(std::shared_ptr<CommandContext> context, const QUuid& itemId);
	bool showContainers(std::shared_ptr<CommandContext> context);
	bool showEquipments(std::shared_ptr<CommandContext> context);
	bool changeContainerId(std::shared_ptr<CommandContext> context, const QUuid& itemId, const QUuid& containerId);
};

ItemsCommand::ItemsCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, ICommand(parent) {
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

bool ItemsCommand::Private::showEntities(std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto itemsService = services->get<ItemsService>();
	auto entities = itemsService->entities();

	ConsoleTable table({ "..", "Id", "Name", "Desc", "Type", "SubType", "w", "h", "rarity", "rows", "cols", "stack" });

	for (auto item : entities) {
		printEntity(table, item);
	}

	controller->print(table);
	return true;
}

bool ItemsCommand::Private::showContainerItems(std::shared_ptr<CommandContext> context, const QUuid& containerId) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto itemsService = services->get<ItemsService>();
	auto items = itemsService->itemsFromContainer(containerId);
	if (items.empty()) {
		controller->printWarning(QString("Container is empty or undefined. %1")
			.arg(containerId
				.toString(QUuid::WithoutBraces)));
		return false;
	}

	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	for (auto item : items) {
		printItem(table, item);
	}

	controller->print(table);
	return true;
}

bool ItemsCommand::Private::showItem(std::shared_ptr<CommandContext> context, const QUuid& itemId) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto itemsService = services->get<ItemsService>();
	auto item = itemsService->item(itemId);
	if (!item) {
		controller->printWarning(QString("Item is undefined. %1")
			.arg(itemId
				.toString(QUuid::WithoutBraces)));
		return false;
	}

	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	printItem(table, item);
	controller->print(table);
	return true;
}

bool ItemsCommand::Private::showContainers(std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto itemsService = services->get<ItemsService>();
	auto items = itemsService->containers();

	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	for (auto item : items) {
		printItem(table, item);
	}

	controller->print(table);
	return true;
}

bool ItemsCommand::Private::showEquipments(std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto itemsService = services->get<ItemsService>();
	auto items = itemsService->equipments();

	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	for (auto item : items) {
		printItem(table, item);
	}

	controller->print(table);
	return true;
}

bool ItemsCommand::Private::changeContainerId(std::shared_ptr<CommandContext> context, const QUuid& itemId, const QUuid& containerId) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto itemsService = services->get<ItemsService>();
	auto item = itemsService->item(itemId);
	if (!item) {
		controller->printWarning(QString("Item is undefined. %1")
			.arg(itemId
			.toString(QUuid::WithoutBraces)));
		return false;
	}

	itemsService->changeItemContainer(item, containerId);
	ConsoleTable table({ "..", "Id", "EntityId", "ContainerId", "Lvl", "Count", "x", "y" });
	printItem(table, item);
	controller->print(table);
	return true;
}

bool ItemsCommand::execute(const std::shared_ptr<Instruction> instruction, std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto action = instruction->text("action");
	if (action.isEmpty()) {
		controller->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "show-container") {
		return d->showContainerItems(context,
			instruction->parameter("id").toUuid());
	}
	else if (action == "show-item") {
		return d->showItem(context,
			instruction->parameter("id").toUuid());
	}
	else if (action == "change-container") {
		return d->changeContainerId(context,
			instruction->parameter("itemId").toUuid(),
			instruction->parameter("containerId").toUuid());
	}
	else if (action == "show-containers") {
		return d->showContainers(context);
	}
	else if (action == "show-equipments") {
		return d->showEquipments(context);
	}
	else if (action == "show-entities") {
		return d->showEntities(context);
	}

	return false;
}
