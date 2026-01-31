#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/commands/command.h"
#include "Game/commands/cmd/help_cmd.h"
#include "Game/app_controller.h"
#include <QRegularExpression>
#include <QDebug>
#include <QHash>
#include <QStringList>
#include <memory>
#include <map>

class CommandProcessor::Private {
public:
	Private(CommandProcessor* parent)
		: q(parent) {
	}

	CommandProcessor* q;

	// Парсинг командной строки с поддержкой кавычек
	static QStringList parseCommandLine(const QString& line) {
		QStringList tokens;
		QString currentToken;
		bool inQuotes = false;
		bool escaped = false;

		for (int i = 0; i < line.size(); ++i) {
			QChar ch = line[i];

			if (escaped) {
				currentToken += ch;
				escaped = false;
				continue;
			}

			if (ch == '\\') {
				escaped = true;
				continue;
			}

			if (ch == '"') {
				inQuotes = !inQuotes;
				continue;
			}

			if (ch.isSpace() && !inQuotes) {
				if (!currentToken.isEmpty()) {
					tokens << currentToken;
					currentToken.clear();
				}
				continue;
			}

			currentToken += ch;
		}

		if (!currentToken.isEmpty() || inQuotes) {
			tokens << currentToken;
		}

		return tokens;
	}

	std::map<QString, std::unique_ptr<ICommand>> commands;
};

CommandProcessor::CommandProcessor(QObject* parent)
	: QObject(parent)
	, d(new Private(this)) {
	// Регистрация встроенных служебных команд
	registerCommand(std::make_unique<HelpCommand>(this));
}

CommandProcessor::~CommandProcessor() = default;

void CommandProcessor::registerCommand(std::unique_ptr<ICommand> command) {
	QString name = command->name().toLower();
	if (d->commands.contains(name)) {
		qWarning() << "Command already registered:" << name;
		return;
	}

	d->commands.emplace(name, std::move(command));
}

bool CommandProcessor::execute(const QString& commandLine, CommandContext* context) {
	if (!context) {
		qWarning() << "CommandContext is null";
		return false;
	}

	auto args = d->parseCommandLine(commandLine.trimmed());
	if (args.isEmpty()) {
		return true;
	}

	QString cmdName = args
		.takeFirst()
		.toLower();

	ICommand* command = findCommandUnsafe(cmdName);

	if (!command) {
		context->printError(QString("Unknown command: '%1'. Type 'help' for available commands.").arg(cmdName));
		emit commandError(cmdName, "Unknown command");
		return false;
	}

	// Проверка количества аргументов
	if (args.size() < command->minArgs()) {
		context->printError(QString("Command '%1' requires at least %2 arguments")
			.arg(cmdName).arg(command->minArgs()));
		context->print(QString("Usage: %1").arg(command->help()));
		emit commandError(cmdName, "Insufficient arguments");
		return false;
	}

	if (command->maxArgs() >= 0 && args.size() > command->maxArgs()) {
		context->printError(QString("Command '%1' accepts at most %2 arguments")
			.arg(cmdName).arg(command->maxArgs()));
		context->print(QString("Usage: %1").arg(command->help()));
		emit commandError(cmdName, "Too many arguments");
		return false;
	}

	// Выполнение
	bool success = false;
	try {
		success = command->execute(context, args);
	}
	catch (const std::exception& e) {
		context->printError(QString("Exception in command '%1': %2").arg(cmdName).arg(e.what()));
		success = false;
	}
	catch (...) {
		context->printError(QString("Unknown exception in command '%1'").arg(cmdName));
		success = false;
	}

	emit commandExecuted(cmdName, success);
	return success;
}

QStringList CommandProcessor::availableCommands() const {
	QStringList result;
	for (auto it = d->commands.cbegin(); it != d->commands.cend(); ++it) {
		result << it->first;
	}
	result.sort();
	return result;
}

ICommand* CommandProcessor::findCommand(const QString& name) const {
	QString key = name.toLower();
	return findCommandUnsafe(key);
}

ICommand* CommandProcessor::findCommandUnsafe(const QString& name) const {
	auto it = d->commands.find(name);
	return (it != d->commands.end()) ? it->second.get() : nullptr;
}

QString CommandProcessor::helpForCommand(const QString& name) const {
	auto cmd = findCommand(name); // используем безопасный метод
	if (!cmd) return QString("Command '%1' not found").arg(name);

	QString helpText = QString("%1 — %2\n").arg(cmd->name()).arg(cmd->description());
	helpText += QString("Usage: %1\n").arg(cmd->help());
	return helpText;
}

QString CommandProcessor::fullHelp() const {
	QString result = "Available commands:\n\n";
	for (const QString& name : availableCommands()) {
		if (auto cmd = d->commands[name].get()) {
			result += QString("%1: %2\n")
				.arg(cmd->name())
				.arg(cmd->description());
		}
	}
	result += "\nType 'help <command>' for detailed information.";
	return result;
}
