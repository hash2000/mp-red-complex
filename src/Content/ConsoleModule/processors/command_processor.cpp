#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/i_command.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Resources/resources.h"

#include <QDebug>
#include <QMap>
#include <QElapsedTimer>
#include <QDomDocument>
#include <QJsonDocument>
#include <memory>
#include <map>

class CommandProcessor::Private {
public:
	Private(CommandProcessor* parent) : q(parent) { }
	CommandProcessor* q;

	Resources* resources;
	std::map<QString, std::unique_ptr<ICommand>> commands;

	static std::shared_ptr<Instruction> parseCommandLine(const QString& line, std::shared_ptr<CommandContext> context);

	ICommand* findCommandUnsafe(const QString& name) const;
	void printElapsedTimeForCommand(const QElapsedTimer& timer, bool succes, std::shared_ptr<CommandContext> context);
	bool executeInstruction(const std::shared_ptr<Instruction> instruction, std::shared_ptr<CommandContext> context);
};

CommandProcessor::CommandProcessor(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
}

CommandProcessor::~CommandProcessor() = default;

std::shared_ptr<Instruction> CommandProcessor::Private::parseCommandLine(const QString& line, std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	auto instruction = std::make_shared<Instruction>();

	int pos = 0;
	const int len = line.size();

	auto skipSpaces = [&]() {
		while (pos < len && line[pos].isSpace()) {
			pos++;
		}
	};

	skipSpaces();
	if (pos >= len) {		
		return nullptr;
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
				controller->printError(QString("Invalid parameter name. pos %1")
					.arg(pos));
				return nullptr;
			}

			++pos;
		}

		if (pos >= len) {
			controller->printError(QString("Parameter without ':' delimiter. pos %1")
				.arg(pos));
			return nullptr;
		}

		const QString paramName = line.mid(nameStart, pos - nameStart);
		if (paramName.isEmpty()) {
			controller->printError(QString("Empty parameter name. pos %1")
				.arg(pos));
			return nullptr;
		}

		// пропускаем ':'
		pos++;

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
			instruction->parametersMimeTypes.insert(paramName, "text/plane");
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
					controller->printError(QString("Invalid JSON: %1. pos %2")
						.arg(error.errorString())
						.arg(pos));
					return nullptr;
				}

				instruction->parameters.insert(paramName, QVariant::fromValue(doc));
				instruction->parametersMimeTypes.insert(paramName, "application/json");
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
					controller->printError(QString("Invalid XML. pos %1")
						.arg(pos));
					return nullptr;
				}

				instruction->parameters.insert(paramName, QVariant::fromValue(doc));
				instruction->parametersMimeTypes.insert(paramName, "application/xml");
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
				instruction->parametersMimeTypes.insert(paramName, "text/x-ini");
				continue;
			}

			// TEXT
			if (rest.size() >= 6 && rest.mid(0, 6).compare(u"<text>", Qt::CaseInsensitive) == 0) {
				pos += 6;
				const int endPos = line.indexOf(u"</text>", pos, Qt::CaseInsensitive);
				QString content = (endPos < 0) ? line.mid(pos) : line.mid(pos, endPos - pos);
				pos = (endPos < 0) ? len : endPos + 7;

				instruction->parameters.insert(paramName, content);
				instruction->parametersMimeTypes.insert(paramName, "text/plane");
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
			instruction->parametersMimeTypes.insert(paramName, "text/plane");
		}
	}

	return instruction;
}

void CommandProcessor::Private::printElapsedTimeForCommand(const QElapsedTimer& timer, bool succes, std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto ms = timer.elapsed();
	const double secs = ms / 1000.0;
	const auto formatted = QString("Cmd elapsed %1 Sec")
		.arg(QString::number(secs, 'f', 2));

	if (succes) {
		controller->printSuccess(formatted);
	}
	else {
		controller->printError(formatted);
	}
}

bool CommandProcessor::Private::executeInstruction(const std::shared_ptr<Instruction> instruction, std::shared_ptr<CommandContext> context) {
	if (!context) {
		qWarning() << "CommandContext is null";
		return false;
	}

	const auto controller = context->commandController();
	const auto cmdName = instruction->command;

	auto command = findCommandUnsafe(cmdName);
	if (!command) {
		controller->printError(QString("Command '%1' not found")
			.arg(cmdName));
		return false;
	}

	// Проверка количества аргументов
	if (instruction->parameters.size() < command->minArgs()) {
		controller->printError(QString("Command '%1' requires at least %2 arguments")
			.arg(cmdName)
			.arg(command->minArgs()));
		controller->print(QString("Usage: %1")
			.arg(command->help()));
		emit q->commandError(cmdName, "Insufficient arguments");
		return false;
	}

	if (command->maxArgs() >= 0 && instruction->parameters.size() > command->maxArgs()) {
		controller->printError(QString("Command '%1' accepts at most %2 arguments")
			.arg(cmdName)
			.arg(command->maxArgs()));
		controller->print(QString("Usage: %1")
			.arg(command->help()));
		emit q->commandError(cmdName, "Too many arguments");
		return false;
	}

	bool success = false;
	try {
		success = command->execute(instruction, context);
	}
	catch (const std::exception& e) {
		controller->printError(QString("Exception in command '%1': %2")
			.arg(cmdName)
			.arg(e.what()));
		success = false;
	}
	catch (...) {
		controller->printError(QString("Unknown exception in command '%1'")
			.arg(cmdName));
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

bool CommandProcessor::executeCommand(const QString& commandName, const QMap<QString, QString>& args, std::shared_ptr<CommandContext> context) {
	auto instruction = std::make_shared<Instruction>();
	instruction->command = commandName;
	for (auto it = args.begin(); it != args.end(); it++) {
		instruction->parameters.insert(it.key(), it.value());
	}

	return d->executeInstruction(instruction, context);
}

bool CommandProcessor::execute(const QString& commandLine, std::shared_ptr<CommandContext> context) {
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
