#include "Content/SystemRouter/commands/system_router_routes_command.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/services/services_registry.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString nodeData(const LazyTreeNodePtr& node) {
	if (!node) {
		return {};
	}

	QJsonObject data;
	const auto values = node->properties();
	for (auto it = values.constBegin(); it != values.constEnd(); it++) {
		QJsonObject property;
		property.insert(QStringLiteral("mime"),
			QJsonArray::fromStringList(node->propertyMimeTypes(it.key())));

		if (node->propertyValueIsBlob(it.key())
			&& !node->propertyValueBlobId(it.key()).isEmpty()) {
			property.insert(QStringLiteral("value_blob"),
				node->propertyValueBlobId(it.key()));
		}
		else {
			property.insert(QStringLiteral("value"),
				QJsonValue::fromVariant(it.value()));
		}

		data.insert(it.key(), property);
	}

	return QString::fromUtf8(
		QJsonDocument(data).toJson(QJsonDocument::Compact));
}

} // namespace

SystemRouterRoutesCommand::SystemRouterRoutesCommand(QObject* parent)
	: ICommand(parent) {
}

SystemRouterRoutesCommand::~SystemRouterRoutesCommand() = default;

QString SystemRouterRoutesCommand::name() const {
	return QStringLiteral("system-router-routes");
}

QString SystemRouterRoutesCommand::description() const {
	return QStringLiteral("Выводит таблицу всех узлов SystemRouter");
}

QString SystemRouterRoutesCommand::help() const {
	return QStringLiteral(
		"system-router-routes\n"
		"Выводит таблицу со всеми записями nodes: id, parent_id, name, data.");
}

bool SystemRouterRoutesCommand::execute(std::shared_ptr<CommandContext> context) {
	if (!context || !context->commandController()) {
		return false;
	}

	auto* controller = context->commandController();
	auto* services = controller->services();
	auto* routerService = services ? services->get<SystemRouterService>() : nullptr;
	if (!routerService) {
		controller->printError(QStringLiteral(
			"Сервис системного маршрутизатора недоступен"));
		return false;
	}

	const auto nodes = routerService->allNodes();
	if (!routerService->lastError().isEmpty()) {
		controller->printError(routerService->lastError());
		return false;
	}

	ConsoleTable table({ QStringLiteral("id"), QStringLiteral("parent_id"),
		QStringLiteral("name"), QStringLiteral("data") });
	for (const auto& node : nodes) {
		table.addRow({
			node->id(),
			node->parentId(),
			node->name(),
			nodeData(node)
			});
	}

	controller->print(table);
	return true;
}
