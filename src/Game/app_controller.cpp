#include "Game/app_controller.h"
#include "Game/mdi_child_window.h"
#include "Game/commands/command.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/commands/cmd/windows_close_all_cmd.h"
#include "Game/commands/cmd/windows_list_cmd.h"
#include "Game/commands/cmd/windows_close_cmd.h"
#include "Game/commands/cmd/window_create_cmd.h"
#include "Game/commands/cmd/states_store_cmd.h"
#include "Game/commands/cmd/items_create.h"
#include "Game/commands/cmd/user_logout_cmd.h"
#include "Game/services.h"
#include "Game/controllers.h"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>
#include <QUuid>
#include <QMetaType>

class ApplicationController::Private {
public:
	Private(ApplicationController* parent)
	: q(parent) {
	}

	ApplicationController* q;
	std::unique_ptr<CommandProcessor> commandProcessor;
	std::unique_ptr<CommandContext> commandContext;
	std::unique_ptr<Services> services;
	std::unique_ptr<Controllers> controllers;
};

ApplicationController::ApplicationController(Resources* resources, QObject* parent)
: QObject(parent)
,	d(std::make_unique<Private>(this)) {

	// Создание процессора команд
	d->commandProcessor = std::make_unique<CommandProcessor>(resources);
	d->commandContext = std::make_unique<CommandContext>(this);
	d->services = std::make_unique<Services>(resources);
	d->controllers = std::make_unique<Controllers>(this, d->services.get());

	// Регистрация встроенных системных команд
	d->commandProcessor->registerCommand(std::make_unique<ListWindowsCommand>());
	d->commandProcessor->registerCommand(std::make_unique<CloseWindowsCommand>());
	d->commandProcessor->registerCommand(std::make_unique<CloseAllWindowsCommand>());
	d->commandProcessor->registerCommand(std::make_unique<CreateWindowCommand>());
	d->commandProcessor->registerCommand(std::make_unique<StatesStoreCommand>());
	d->commandProcessor->registerCommand(std::make_unique<ItemsCreateCommand>());
	d->commandProcessor->registerCommand(std::make_unique<UserLogoutCommand>());

	d->services->run();

	qInfo() << "ApplicationController initialized with"
		<< d->commandProcessor->availableCommands().size()
		<< "available commands";
}

ApplicationController::~ApplicationController() {
	qInfo() << "ApplicationController destroyed";
}


CommandProcessor* ApplicationController::commandProcessor() const {
	return d->commandProcessor.get();
}

CommandContext* ApplicationController::commandContext() const {
	return d->commandContext.get();
}

Services* ApplicationController::services() const {
	return d->services.get();
}

Controllers* ApplicationController::controllers() const {
	return d->controllers.get();
}

bool ApplicationController::executeCommand(const QString& commandText, QObject* requester) {
	Q_UNUSED(requester); // Может использоваться для аудита/логгирования

	if (!d->commandProcessor || commandText.trimmed().isEmpty()) {
		return false;
	}

	QElapsedTimer timer;
	timer.start();

	bool success = d->commandProcessor->execute(commandText, d->commandContext.get());

	qint64 elapsed = timer.elapsed();
	QString cmdName = commandText.split(' ', Qt::SkipEmptyParts).value(0);

	if (success) {
		emit commandExecuted(cmdName, elapsed);
	}
	else {
		emit commandFailed(cmdName, "Execution failed or command not found");
	}

	return success;
}

bool ApplicationController::executeCommandByName(const QString& commandName,
	const QStringList& args,
	QObject* requester) {
	Q_UNUSED(requester);

	if (!d->commandProcessor) {
		return false;
	}

	auto command = d->commandProcessor->findCommand(commandName);
	if (!command) {
		emit commandFailed(commandName, "Command not found");
		return false;
	}

	QElapsedTimer timer;
	timer.start();

	bool success = false;
	try {
		success = command->execute(d->commandContext.get(), args);
	}
	catch (const std::exception& e) {
		emit commandFailed(commandName, QString::fromUtf8(e.what()));
		return false;
	}

	qint64 elapsed = timer.elapsed();
	if (success) {
		emit commandExecuted(commandName, elapsed);
	}
	else {
		emit commandFailed(commandName, "Command execution returned false");
	}

	return success;
}
