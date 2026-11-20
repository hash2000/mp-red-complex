#include "Content/SystemRouter/commands/system_router_blobs_command.h"
#include "Content/SystemRouter/services/system_router_blob_service.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/services/services_registry.h"

SystemRouterBlobsCommand::SystemRouterBlobsCommand(QObject* parent)
	: ICommand(parent) {
}

SystemRouterBlobsCommand::~SystemRouterBlobsCommand() = default;

QString SystemRouterBlobsCommand::name() const {
	return QStringLiteral("system-router-blobs");
}

QString SystemRouterBlobsCommand::description() const {
	return QStringLiteral("Выводит таблицу всех блоков данных SystemRouter");
}

QString SystemRouterBlobsCommand::help() const {
	return QStringLiteral(
		"system-router-blobs\n"
		"Выводит таблицу со всеми записями data_blobs: id, version, description, data.");
}

bool SystemRouterBlobsCommand::execute(std::shared_ptr<CommandContext> context) {
	if (!context || !context->commandController()) {
		return false;
	}

	auto* controller = context->commandController();
	auto* services = controller->services();
	auto* blobService = services ? services->get<SystemRouterBlobService>() : nullptr;
	if (!blobService) {
		controller->printError(QStringLiteral(
			"Сервис блоков данных SystemRouter недоступен"));
		return false;
	}

	const auto blobs = blobService->blobs();
	if (!blobService->lastError().isEmpty()) {
		controller->printError(blobService->lastError());
		return false;
	}

	ConsoleTable table({ QStringLiteral("id"), QStringLiteral("version"),
		QStringLiteral("description"), QStringLiteral("data") });
	table.setColumnAlignment(QStringLiteral("version"), ConsoleTable::Right);

	for (const auto& blob : blobs) {
		table.addRow({
			blob.id,
			blob.version,
			blob.description,
			blob.data
			});
	}

	controller->print(table);
	return true;
}
