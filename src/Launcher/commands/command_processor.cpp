#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/i_command.h"
#include "Launcher/commands/instruction.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/commands/cmd/help_cmd.h"
#include "Launcher/app_controller.h"
#include "Launcher/mdi_child_window.h"
#include "Launcher/controllers.h"
#include "Libs/Resources/resources.h"
#include "Libs/Resources/variables/variables_context.h"

#include <QDebug>
#include <QMap>
#include <QElapsedTimer>
#include <memory>
#include <map>

class CommandProcessor::Private {
public:
	Private(CommandProcessor* parent) : q(parent) { }
	CommandProcessor* q;

	Resources* resources;
	std::map<QString, std::unique_ptr<ICommand>> commands;

	static std::shared_ptr<Instruction> parseCommandLine(const QString& line, CommandContext* context);

	ICommand* findCommandUnsafe(const QString& name) const;
	void printElapsedTimeForCommand(const QElapsedTimer& timer, bool succes, CommandContext* context);
	bool executeInstruction(const std::shared_ptr<Instruction> instruction, CommandContext* context);
};

CommandProcessor::CommandProcessor(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;

	// Регистрация встроенных служебных команд
	registerCommand(std::make_unique<HelpCommand>(this));
}

CommandProcessor::~CommandProcessor() = default;

std::shared_ptr<Instruction> CommandProcessor::Private::parseCommandLine(const QString& line, CommandContext* context) {
	auto instruction = std::make_shared<Instruction>();

	int pos = 0;
	const int len = line.size();

	auto skipSpaces = [&]() {
		while (pos < len && line[pos].isSpace()) {
			++pos;
		}
	};

	skipSpaces();
	if (pos >= len) {		
		return std::shared_ptr<Instruction>();
	}

	// Имя команды
	const int cmdStart = pos;
	while (pos < len && !line[pos].isSpace()) {
		++pos;
	}

	instruction->command = line.mid(cmdStart, pos - cmdStart);

	// Параметры
	while (pos < len) {
		skipSpaces();
		if (pos >= len) {
			break;
		}

		const int nameStart = pos;
		while (pos < len && line[pos] != u':') {
			if (line[pos].isSpace()) {
				context->printError(QString("Invalid parameter name. pos %1")
					.arg(pos));
				return std::shared_ptr<Instruction>();
			}

			++pos;
		}

		if (pos >= len) {
			context->printError(QString("Parameter without ':' delimiter. pos %1")
				.arg(pos));
			return std::shared_ptr<Instruction>();
		}

		const QString paramName = line.mid(nameStart, pos - nameStart);
		if (paramName.isEmpty()) {
			context->printError(QString("Empty parameter name. pos %1")
				.arg(pos));
			return std::shared_ptr<Instruction>();
		}

		++pos; // пропускаем ':'

		if (pos >= len) {
			instruction->parameters.insert(paramName, QVariant{ });
			break;
		}

		const QChar first = line[pos];

		// Кавычки
		if (first == u'"') {
			++pos;
			QString value;
			bool escaped = false;

			while (pos < len) {
				const QChar ch = line[pos];
				if (escaped) {
					value += ch;
					escaped = false;
					++pos;
				}
				else if (ch == u'\\') {
					escaped = true;
					++pos;
				}
				else if (ch == u'"') {
					++pos;
					break;
				}
				else {
					value += ch;
					++pos;
				}
			}

			instruction->parameters.insert(paramName, QVariant::fromValue(value));
			continue;
		}

		// Блочные форматы
		if (first == u'<') {
			const QStringView rest = QStringView{ line }.mid(pos);

			// JSON
			if (rest.size() >= 6 && rest.mid(0, 6).compare(u"<json>", Qt::CaseInsensitive) == 0) {
				pos += 6;
				const int endPos = line.indexOf(u"</json>", pos, Qt::CaseInsensitive);
				QString content = (endPos < 0) ? line.mid(pos) : line.mid(pos, endPos - pos);
				pos = (endPos < 0) ? len : endPos + 7;

				QJsonParseError error;
				QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8(), &error);
				if (error.error != QJsonParseError::NoError) {
					context->printError(QString("Invalid JSON: %1. pos %2")
						.arg(error.errorString())
						.arg(pos));
					return std::shared_ptr<Instruction>();
				}

				instruction->parameters.insert(paramName, QVariant::fromValue(doc));
				continue;
			}

			// XML
			if (rest.size() >= 5 && rest.mid(0, 5).compare(u"<xml>", Qt::CaseInsensitive) == 0) {
				pos += 5;
				const int endPos = line.indexOf(u"</xml>", pos, Qt::CaseInsensitive);
				QString content = (endPos < 0) ? line.mid(pos) : line.mid(pos, endPos - pos);
				pos = (endPos < 0) ? len : endPos + 6;

				QDomDocument doc;
				if (!doc.setContent(content)) {
					context->printError(QString("Invalid XML. pos %1")
						.arg(pos));
					return std::shared_ptr<Instruction>();
				}

				instruction->parameters.insert(paramName, QVariant::fromValue(doc));
				continue;
			}

			// INI
			if (rest.size() >= 5 && rest.mid(0, 5).compare(u"<ini>", Qt::CaseInsensitive) == 0) {
				pos += 5;
				const int endPos = line.indexOf(u"</ini>", pos, Qt::CaseInsensitive);
				QString content = (endPos < 0) ? line.mid(pos) : line.mid(pos, endPos - pos);
				pos = (endPos < 0) ? len : endPos + 6;

				// Простой парсер INI
				QMap<QString, QVariant> iniMap;
				const QStringList lines = content.split('\n', Qt::SkipEmptyParts);
				for (const QString& iniLine : lines) {
					const int eqPos = iniLine.indexOf('=');
					if (eqPos > 0) {
						const QString key = iniLine.left(eqPos).trimmed();
						const QString value = iniLine.mid(eqPos + 1).trimmed();
						iniMap.insert(key, value);
					}
				}

				instruction->parameters.insert(paramName, QVariant::fromValue(iniMap));
				continue;
			}

			// TEXT
			if (rest.size() >= 6 && rest.mid(0, 6).compare(u"<text>", Qt::CaseInsensitive) == 0) {
				pos += 6;
				const int endPos = line.indexOf(u"</text>", pos, Qt::CaseInsensitive);
				QString content = (endPos < 0) ? line.mid(pos) : line.mid(pos, endPos - pos);
				pos = (endPos < 0) ? len : endPos + 7;

				instruction->parameters.insert(paramName, content);
				continue;
			}
		}

		// Обычный текст
		{
			QString value;
			bool escaped = false;

			while (pos < len) {
				const QChar ch = line[pos];
				if (escaped) {
					value += ch;
					escaped = false;
					++pos;
				}
				else if (ch == u'\\') {
					escaped = true;
					++pos;
				}
				else if (ch.isSpace()) {
					break;
				}
				else {
					value += ch;
					++pos;
				}
			}

			instruction->parameters.insert(paramName, QVariant::fromValue(value));
		}
	}

	return instruction;
}

void CommandProcessor::Private::printElapsedTimeForCommand(const QElapsedTimer& timer, bool succes, CommandContext* context) {
	const auto ms = timer.elapsed();
	const double secs = ms / 1000.0;
	const auto formatted = QString("Cmd elapsed %1 Sec")
		.arg(QString::number(secs, 'f', 2));

	if (succes) {
		context->printSuccess(formatted);
	}
	else {
		context->printError(formatted);
	}
}

bool CommandProcessor::Private::executeInstruction(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	if (!context) {
		qWarning() << "CommandContext is null";
		return false;
	}

	QString cmdName = instruction->command;

	auto command = findCommandUnsafe(cmdName);
	if (!command) {
		context->printError(QString("Command '%1' not found")
			.arg(cmdName));
		return false;
	}

	// Проверка количества аргументов
	if (instruction->parameters.size() < command->minArgs()) {
		context->printError(QString("Command '%1' requires at least %2 arguments")
			.arg(cmdName)
			.arg(command->minArgs()));
		context->print(QString("Usage: %1").arg(command->help()));
		emit q->commandError(cmdName, "Insufficient arguments");
		return false;
	}

	if (command->maxArgs() >= 0 && instruction->parameters.size() > command->maxArgs()) {
		context->printError(QString("Command '%1' accepts at most %2 arguments")
			.arg(cmdName)
			.arg(command->maxArgs()));
		context->print(QString("Usage: %1").arg(command->help()));
		emit q->commandError(cmdName, "Too many arguments");
		return false;
	}

	bool success = false;
	try {
		success = command->execute(instruction, context);
	}
	catch (const std::exception& e) {
		context->printError(QString("Exception in command '%1': %2").arg(cmdName).arg(e.what()));
		success = false;
	}
	catch (...) {
		context->printError(QString("Unknown exception in command '%1'").arg(cmdName));
		success = false;
	}

	emit q->commandExecuted(cmdName, success);
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

bool CommandProcessor::executeCommand(const QString& commandName, const QMap<QString, QString>& args,
	CommandContext* context) {
	auto instruction = std::make_shared<Instruction>();
	instruction->command = commandName;
	for (auto it = args.begin(); it != args.end(); it++) {
		instruction->parameters.insert(it.key(), it.value());
	}

	return d->executeInstruction(instruction, context);
}

bool CommandProcessor::execute(const QString& commandLine, CommandContext* context) {
	QElapsedTimer timer;
	timer.start();

	auto instruction = Private::parseCommandLine(commandLine.trimmed(), context);
	if (!instruction) {
		d->printElapsedTimeForCommand(timer, false, context);
		return true;
	}

	auto result = d->executeInstruction(instruction, context);
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

QString CommandProcessor::helpForCommand(const QString& name) const {
	auto cmd = findCommand(name); // используем безопасный метод
	if (!cmd) return QString("Command '%1' not found").arg(name);

	QString helpText = QString("%1 — %2\n").arg(cmd->name()).arg(cmd->description());
	helpText += QString("Usage: %1\n").arg(cmd->help());
	return helpText;
}

QString CommandProcessor::fullHelp() const {
	QString result = "=======================\nAvailable commands:\n\n";
	for (const auto& name : availableCommands()) {
		if (auto cmd = d->commands[name].get()) {
			result += QString("%1: %2\n")
				.arg(cmd->name())
				.arg(cmd->description());
		}
	}

	result += "=======================\nAvailable variables:\n\n";

	for (const auto& name : d->resources->Variables.available()) {
		const auto value = d->resources->Variables.get(name, "")
			.toString();
		result += QString("%1: %2\n")
			.arg(name)
			.arg(value);
	}
	result += "\nType 'help <command>' for detailed information.";
	return result;
}
