#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/i_command.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"

class CommandController::Private {
public:
	Private(CommandController* parent) : q(parent) {}
	CommandController* q;

	std::unique_ptr<CommandContext> commandContext;
	Resources* resources;
};

CommandController::CommandController(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {

	d->resources = resources;

	// Создание процессора команд
	d->commandContext = std::make_unique<CommandContext>(this, nullptr /*is global context*/);
}

CommandController::~CommandController() {
	qInfo() << "CommandController destroyed";
}

void CommandController::init() {
	initContext();

	d->commandContext->printSuccess(QString("CommandController initialized with %1 available commands")
		.arg(commandProcessor()->availableCommands().size()));
}

CommandContext* CommandController::commandContext() const {
	return d->commandContext.get();
}

Resources* CommandController::resources() const {
	return d->resources;
}

bool CommandController::execute(const QString& commandText, QObject* requester) {
	return commandProcessor()->execute(commandText, d->commandContext.get());
}

bool CommandController::executeCommand(
	const QString& commandName,
	const QMap<QString, QString>& args,
	QObject* requester) {
	return commandProcessor()->executeCommand(commandName, args, d->commandContext.get());
}
