#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
#include "Launcher/widgets_builder.h"
#include "Libs/Engine/scripting/i_command.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/services/services_registry.h"

#include "Libs/Engine/commands/widget_zone_command.h"

#include "Content/ConsoleModule/commands/help_cmd.h"
#include "Content/FetchApiModule/commands/fetch_api_cmd.h"
#include "Content/UsersModule/commands/users_cmd.h"
#include "Content/ConsoleModule/commands/echo_cmd.h"

class ApplicationController::Private {
public:
	Private(ApplicationController* parent) : q(parent) {}
	ApplicationController* q;

	std::unique_ptr<CommandProcessor> commandProcessor;
	std::unique_ptr<Controllers> controllers;
	std::unique_ptr<Services> servicesBuilder;
	std::unique_ptr<WidgetsBuilder> widgetsBuilder;
	std::unique_ptr<ServicesRegistry> services;
};

ApplicationController::ApplicationController(Resources* resources, QObject* parent)
	: CommandController(resources, parent)
	, d(std::make_unique<Private>(this)) {
}

ApplicationController::~ApplicationController() = default;

void ApplicationController::initContext() {
	d->commandProcessor = std::make_unique<CommandProcessor>(resources());
	d->servicesBuilder = std::make_unique<Services>(resources(), globalContext());
	d->controllers = std::make_unique<Controllers>(this);
	d->widgetsBuilder = std::make_unique<WidgetsBuilder>(this);

	auto commands = commandProcessor();

	commands->registerCommand(std::make_unique<HelpCommand>(this));
	commands->registerCommand(std::make_unique<EchoCommand>(this));
	commands->registerCommand(std::make_unique<WidgetZoneCommand>("documents", MainWindow::WidgetKind::Document, this));
	commands->registerCommand(std::make_unique<WidgetZoneCommand>("info-panels", MainWindow::WidgetKind::BottomPanel, this));
	commands->registerCommand(std::make_unique<WidgetZoneCommand>("side-panels", MainWindow::WidgetKind::SidePanel, this));
	commands->registerCommand(std::make_unique<UsersCommand>(this));
	commands->registerCommand(std::make_unique<FetchApiCommand>(this));
}

CommandProcessor* ApplicationController::commandProcessor() const {
	return d->commandProcessor.get();
}

ServicesRegistry* ApplicationController::services() const {
	if (!d->services) {
		d->services = d->servicesBuilder->create();
	}
	return d->services.get();
}

Controllers* ApplicationController::controllers() const {
	return d->controllers.get();
}
