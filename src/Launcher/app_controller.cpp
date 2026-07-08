#include "Launcher/app_controller.h"
#include "Content/ConsoleModule/i_command.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Launcher/commands/windows.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
#include "Content/CharactersModule/commands/characters_cmd.h"

class ApplicationController::Private {
public:
	Private(ApplicationController* parent) : q(parent) {}
	ApplicationController* q;

	std::unique_ptr<Controllers> controllers;
	std::unique_ptr<Services> services;
};

ApplicationController::ApplicationController(Resources* resources, QObject* parent)
	: d(std::make_unique<Private>(this))
	, CommandController(resources, parent) {
	d->controllers = std::make_unique<Controllers>(this);
	d->services = std::make_unique<Services>(resources);
}

ApplicationController::~ApplicationController() = default;

void ApplicationController::initContext() {
	auto commands = commandProcessor();

	//commands->registerCommand(std::make_unique<WindowsCommand>(this));
	//commands->registerCommand(std::make_unique<StatesStoreCommand>(this));
	//commands->registerCommand(std::make_unique<ItemsCommand>(this));
	//commands->registerCommand(std::make_unique<UsersCommand>(this));
	commands->registerCommand(std::make_unique<CharactersCommand>(this));
	//commands->registerCommand(std::make_unique<FetchApiCommand>(this));
}

std::unique_ptr<ServicesRegistry> ApplicationController::createServices() {
	return d->services->create();
}

Controllers* ApplicationController::controllers() const {
	return d->controllers.get();
}
