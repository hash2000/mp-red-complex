#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/i_command.h"
#include "Libs/Engine/scripting/parser/commands/parser_command.h"
#include "Libs/Engine/scripting/parser/i_ast.h"
#include "Libs/Engine/scripting/parser/source_cursor.h"
#include "Libs/Resources/resources.h"

#include <QDebug>
#include <QElapsedTimer>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

class CommandProcessor::Private {
public:
	/// Разобранный параметр вызова команды.
	struct Parameter {
		QString name;
		QVariant value;
		QString mimeType;
	};

	Private(CommandProcessor* parent) : q(parent) { }
	CommandProcessor* q;

	Resources* resources;
	std::map<QString, std::unique_ptr<ICommand>> commands;

	static IAstPtr parseCommandLine(const QString& line, std::optional<ParseError>& error);
	static std::vector<Parameter> parametersOf(const IAstPtr& ast);

	ICommand* findCommandUnsafe(const QString& name) const;
	void printElapsedTimeForCommand(const QElapsedTimer& timer, bool success, std::shared_ptr<CommandContext> context);

	/// Создаёт контекст команды: дочерний к `parent`, с именем команды и режимом Mutable.
	std::shared_ptr<CommandContext> createCommandContext(const QString& name,
		const std::vector<Parameter>& parameters,
		std::shared_ptr<CommandContext> parent);

	/// Выполняет команду с уже разобранными параметрами.
	bool executeWithParameters(const QString& name,
		const std::vector<Parameter>& parameters,
		std::shared_ptr<CommandContext> context);
};

CommandProcessor::CommandProcessor(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

CommandProcessor::~CommandProcessor() = default;

IAstPtr CommandProcessor::Private::parseCommandLine(const QString& line, std::optional<ParseError>& error) {
	SourceCursor cursor(line);
	return ParserCommand().parse(cursor, error);
}

std::vector<CommandProcessor::Private::Parameter> CommandProcessor::Private::parametersOf(const IAstPtr& ast) {
	std::vector<Parameter> result;

	for (const auto& node : ast->children("parameter")) {
		Parameter parameter;
		parameter.name = node->parameter("name").toString();
		parameter.value = node->parameter("value");
		parameter.mimeType = node->parameter("mime-type").toString();
		result.push_back(std::move(parameter));
	}

	return result;
}

void CommandProcessor::Private::printElapsedTimeForCommand(const QElapsedTimer& timer, bool success, std::shared_ptr<CommandContext> context) {
	if (!context) {
		return;
	}

	const auto controller = context->commandController();
	const auto ms = timer.elapsed();
	const double secs = ms / 1000.0;
	const auto formatted = QString("Cmd elapsed %1 Sec")
		.arg(QString::number(secs, 'f', 2));

	if (success) {
		controller->printSuccess(formatted);
	}
	else {
		controller->printError(formatted);
	}
}

std::shared_ptr<CommandContext> CommandProcessor::Private::createCommandContext(const QString& name,
	const std::vector<Parameter>& parameters,
	std::shared_ptr<CommandContext> parent) {
	if (!parent) {
		return nullptr;
	}

	// Контекст живёт только на время вызова команды, поэтому не регистрируется
	// в глобальном дереве: повторные вызовы не конфликтуют по namespace.
	auto commandContext = std::make_shared<CommandContext>(parent->commandController(), parent, name);

	// Параметры пишутся как собственные переменные контекста команды,
	// после чего контекст открывается для изменения переменных базовых контекстов.
	for (const auto& parameter : parameters) {
		commandContext->setVariable(parameter.name, parameter.value);
		commandContext->setVariableMimeType(parameter.name, parameter.mimeType);
	}

	commandContext->setVariablesMode(CommandContext::VariablesMode::Mutable);
	return commandContext;
}

bool CommandProcessor::Private::executeWithParameters(const QString& name,
	const std::vector<Parameter>& parameters,
	std::shared_ptr<CommandContext> context) {
	if (!context) {
		qWarning() << "CommandContext is null";
		return false;
	}

	const auto controller = context->commandController();

	auto command = findCommandUnsafe(name.toLower());
	if (!command) {
		controller->printError(QString("Command '%1' not found")
			.arg(name));
		return false;
	}

	// Проверка количества аргументов
	const auto argumentsCount = static_cast<int>(parameters.size());
	if (argumentsCount < command->minArgs()) {
		controller->printError(QString("Command '%1' requires at least %2 arguments")
			.arg(name)
			.arg(command->minArgs()));
		controller->print(QString("Usage: %1")
			.arg(command->help()));
		emit q->commandError(name, "Insufficient arguments");
		return false;
	}

	if (command->maxArgs() >= 0 && argumentsCount > command->maxArgs()) {
		controller->printError(QString("Command '%1' accepts at most %2 arguments")
			.arg(name)
			.arg(command->maxArgs()));
		controller->print(QString("Usage: %1")
			.arg(command->help()));
		emit q->commandError(name, "Too many arguments");
		return false;
	}

	const auto commandContext = createCommandContext(name, parameters, context);
	if (!commandContext) {
		controller->printError(QString("Can't create context for command '%1'")
			.arg(name));
		return false;
	}

	bool success = false;
	try {
		success = command->execute(commandContext);
	}
	catch (const std::exception& e) {
		controller->printError(QString("Exception in command '%1': %2")
			.arg(name)
			.arg(e.what()));
		success = false;
	}
	catch (...) {
		controller->printError(QString("Unknown exception in command '%1'")
			.arg(name));
		success = false;
	}

	emit q->commandExecuted(name, success);
	return success;
}

void CommandProcessor::registerCommand(std::unique_ptr<ICommand> command) {
	QString name = command->name().toLower();
	if (d->commands.contains(name)) {
		qWarning() << "Command already registered:" << name;
		return;
	}

	d->commands.emplace(name, std::move(command));
}

bool CommandProcessor::executeCommand(const QString& commandName, const QMap<QString, QString>& args, std::shared_ptr<CommandContext> context) {
	std::vector<Private::Parameter> parameters;
	parameters.reserve(static_cast<size_t>(args.size()));

	for (auto it = args.cbegin(); it != args.cend(); it++) {
		parameters.push_back({ it.key(), it.value(), QString("text/plane") });
	}

	return d->executeWithParameters(commandName, parameters, context);
}

bool CommandProcessor::execute(const QString& commandLine, std::shared_ptr<CommandContext> context) {
	QElapsedTimer timer;
	timer.start();

	std::optional<ParseError> error;
	auto ast = Private::parseCommandLine(commandLine.trimmed(), error);
	if (!ast) {
		if (error.has_value() && context) {
			context->commandController()->printError(
				QString("Parse error at line %1, column %2: %3")
					.arg(error->location.line)
					.arg(error->location.column)
					.arg(error->message));
		}

		d->printElapsedTimeForCommand(timer, false, context);
		return true;
	}

	const auto commandName = ast->parameter("name").toString();
	auto result = d->executeWithParameters(commandName, Private::parametersOf(ast), context);
	d->printElapsedTimeForCommand(timer, result, context);
	return result;
}

QStringList CommandProcessor::availableCommands() const {
	QStringList result;
	for (auto it = d->commands.cbegin(); it != d->commands.cend(); it++) {
		result << it->first;
	}
	return result;
}

ICommand* CommandProcessor::findCommand(const QString& name) const {
	QString key = name.toLower();
	return d->findCommandUnsafe(key);
}

ICommand* CommandProcessor::Private::findCommandUnsafe(const QString& name) const {
	auto it = commands.find(name);
	return (it != commands.end()) ? it->second.get() : nullptr;
}
