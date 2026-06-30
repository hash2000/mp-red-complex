#include "Launcher/app_controller.h"
#include "Launcher/mdi_child_window.h"
#include "Launcher/commands/command.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
#include "Launcher/commands/cmd/windows_close_all_cmd.h"
#include "Launcher/commands/cmd/windows_list_cmd.h"
#include "Launcher/commands/cmd/windows_close_cmd.h"
#include "Launcher/commands/cmd/window_create_cmd.h"
#include "Launcher/commands/cmd/states_store_cmd.h"
#include "Launcher/commands/cmd/items_cmd.h"
#include "Launcher/commands/cmd/users_cmd.h"
#include "Launcher/commands/cmd/window_invoke_cmd.h"
#include "Launcher/commands/cmd/fetch_api_cmd.h"
#include "Launcher/commands/cmd/characters_cmd.h"

#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTimer>
#include <QDebug>
#include <QElapsedTimer>
#include <QUuid>
#include <QMetaType>

class ApplicationController::Private {
public:
	Private(ApplicationController* parent) : q(parent) { }
	ApplicationController* q;

	std::unique_ptr<CommandProcessor> commandProcessor;
	std::unique_ptr<CommandContext> commandContext;
	Resources* resources;
};

ApplicationController::ApplicationController(Resources* resources, QObject* parent)
: QObject(parent)
,	d(std::make_unique<Private>(this)) {

	d->resources = resources;

	// Создание процессора команд
	d->commandProcessor = std::make_unique<CommandProcessor>(resources);
	d->commandContext = std::make_unique<CommandContext>(this, nullptr /*is global context*/);

	// Регистрация встроенных системных команд
	d->commandProcessor->registerCommand(std::make_unique<ListWindowsCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<CloseWindowsCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<CloseAllWindowsCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<CreateWindowCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<StatesStoreCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<ItemsCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<UsersCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<WindowInvokeCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<CharactersCommand>(this));
	d->commandProcessor->registerCommand(std::make_unique<FetchApiCommand>(this));

	d->commandContext->services()->run();

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

Resources* ApplicationController::resources() const {
	return d->resources;
}

bool ApplicationController::execute(const QString& commandText, QObject* requester) {
	return d->commandProcessor->execute(commandText, d->commandContext.get());
}

bool ApplicationController::executeCommand(const QString& commandName, const QMap<QString, QString>& args,
	QObject* requester) {
	return d->commandProcessor->executeCommand(commandName, args, d->commandContext.get());
}
