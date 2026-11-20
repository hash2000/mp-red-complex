#include "Content/SystemRouter/commands/system_router_save_blob_command.h"
#include "Content/SystemRouter/services/system_router_blob_service.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/services/services_registry.h"

SystemRouterSaveBlobCommand::SystemRouterSaveBlobCommand(QObject* parent)
	: ICommand(parent) {
}

SystemRouterSaveBlobCommand::~SystemRouterSaveBlobCommand() = default;

QString SystemRouterSaveBlobCommand::name() const {
	return QStringLiteral("system-router-save-blob");
}

QString SystemRouterSaveBlobCommand::description() const {
	return QStringLiteral("Сохраняет данные code-editor в блок SystemRouter");
}

QString SystemRouterSaveBlobCommand::help() const {
	return QStringLiteral(
		"system-router-save-blob\n"
		"Читает переменные blobId и data из контекста команды.");
}

bool SystemRouterSaveBlobCommand::execute(std::shared_ptr<CommandContext> context) {
	if (!context || !context->commandController()) {
		return false;
	}

	auto* controller = context->commandController();
	auto* services = controller->services();
	auto* blobService = services ? services->get<SystemRouterBlobService>() : nullptr;
	if (!blobService) {
		controller->printError(QStringLiteral("Сервис блоков данных SystemRouter недоступен"));
		return false;
	}

	const auto blobId = context->variable(QStringLiteral("blobId")).toString();
	const auto data = context->variable(QStringLiteral("data")).toString();
	if (blobId.trimmed().isEmpty()) {
		controller->printError(QStringLiteral("Не задан blobId для сохранения данных"));
		return false;
	}

	if (!blobService->saveData(blobId, data)) {
		const auto error = blobService->lastError().isEmpty()
			? QStringLiteral("Не удалось сохранить блок данных")
			: blobService->lastError();
		controller->printError(error);
		return false;
	}

	controller->printSuccess(QStringLiteral("Данные сохранены в блок %1").arg(blobId));
	return true;
}
