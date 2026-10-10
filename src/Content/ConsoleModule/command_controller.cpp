#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/i_command.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_json.h"

#include <QUuid>

class CommandController::Private {
public:
	Private(CommandController* parent) : q(parent) {}
	CommandController* q;

	std::shared_ptr<CommandContext> globalContext;
	Resources* resources;

	std::shared_ptr<CommandContext> createLocalContext();
};

CommandController::CommandController(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {

	d->resources = resources;
	d->globalContext = std::make_shared<CommandContext>(this, std::shared_ptr<CommandContext>() /*is global context*/, "");
}

CommandController::~CommandController() {
	qInfo() << "CommandController destroyed";
}

void CommandController::init() {
	initContext();

	const auto controller = d->globalContext->commandController();
	controller->printSuccess(QString("CommandController initialized with %1 available commands")
		.arg(commandProcessor()->availableCommands().size()));
}

Resources* CommandController::resources() const {
	return d->resources;
}

std::shared_ptr<CommandContext> CommandController::globalContext() {
	return d->globalContext;
}

bool CommandController::execute(const QString& commandText, QObject* requester) {
	return commandProcessor()->execute(commandText, d->createLocalContext());
}

bool CommandController::executeCommand(const QString& commandName, const QMap<QString, QString>& args, QObject* requester) {
	return commandProcessor()->executeCommand(commandName, args, d->createLocalContext());
}

std::shared_ptr<CommandContext> CommandController::Private::createLocalContext() {
	auto nsLocal = QUuid::createUuid().toString(QUuid::WithoutBraces)
		.remove('-');
	return globalContext->createScopedContext(nsLocal);
}

void CommandController::print(const QString& message, const QString& styleClass, const QString& type) {
	QMetaObject::invokeMethod(this, [this, message, styleClass, type]() {
		emit outputRequested(message, styleClass, type);
	}, Qt::QueuedConnection);
}

void CommandController::print(const ConsoleTable& table, const QString& styleClass) {
	const auto html = table.toHtml();
	if (html.isEmpty()) {
		return;
	}

	print(html, styleClass, kCommandPrintStyle_Table);
}

void CommandController::print(const ConsoleJson& json, const QString& styleClass) {
	const auto html = json.toHtml();
	if (html.isEmpty()) {
		return;
	}

	print(html, styleClass, kCommandPrintStyle_Json);
}

void CommandController::printSystem(const QString& message) {
	print(message, "system");
}

void CommandController::printError(const QString& message) {
	print(message, "error");
}

void CommandController::printSuccess(const QString& message) {
	print(message, "success");
}

void CommandController::printWarning(const QString& message) {
	print(message, "warning");
}
