#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_json.h"
#include "Libs/Engine/services/services_registry.h"

class CommandContext::Private {
public:
	Private(CommandContext* parent) : q(parent) { }
	CommandContext* q;

	CommandController* commandController = nullptr;
	CommandContext* globalContext = nullptr;
	std::unique_ptr<ServicesRegistry> services;
};

CommandContext::CommandContext(CommandController* commandController,
	CommandContext* globalContext,
	QObject* parent)
: d(std::make_unique<Private>(this))
, QObject(parent) {
	d->globalContext = globalContext;
	d->commandController = commandController;	
}

CommandContext::~CommandContext() = default;

CommandController* CommandContext::commandController() const {
	return d->commandController;
}

std::unique_ptr<CommandContext> CommandContext::createScopedContext() {
	return std::make_unique<CommandContext>(d->commandController, globalContext());
}

CommandContext* CommandContext::globalContext() {
	return d->globalContext == nullptr ? this : d->globalContext;
}

bool CommandContext::isGlobalContext() const {
	return d->globalContext == nullptr;
}

ServicesRegistry* CommandContext::services() {
	if (!d->services) {
		d->services = d->commandController->createServices();
	}

	return d->services.get();
}

void CommandContext::print(const QString& message, const QString& styleClass, const QString& type) {
	QMetaObject::invokeMethod(this, [this, message, styleClass, type]() {
		emit outputRequested(message, styleClass, type);
		}, Qt::QueuedConnection);
}

void CommandContext::print(const ConsoleTable& table, const QString& styleClass) {
	const auto html = table.toHtml();
	if (html.isEmpty()) {
		return;
	}

	print(html, styleClass, kCommandPrintStyle_Table);
}

void CommandContext::print(const ConsoleJson& json, const QString& styleClass) {
	const auto html = json.toHtml();
	if (html.isEmpty()) {
		return;
	}

	print(html, styleClass, kCommandPrintStyle_Json);
}

void CommandContext::printSystem(const QString& message) {
	print(message, "system");
}

void CommandContext::printError(const QString& message) {
	print(message, "error");
}

void CommandContext::printSuccess(const QString& message) {
	print(message, "success");
}

void CommandContext::printWarning(const QString& message) {
	print(message, "warning");
}
