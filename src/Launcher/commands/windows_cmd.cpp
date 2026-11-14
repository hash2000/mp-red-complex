#include "Launcher/commands/windows_cmd.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
#include "Launcher/commands/windows/windows_close.h"
#include "Launcher/commands/windows/windows_list.h"
#include "Launcher/commands/windows/windows_create.h"
#include "Launcher/commands/windows/windows_invoke.h"

#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/console/console_image.h"
#include "Libs/Engine/services/services_registry.h"

class WindowsCommand::Private {
public:
	Private(WindowsCommand* parent) : q(parent) {}
	WindowsCommand* q;

	Controllers* controllers;
};

WindowsCommand::WindowsCommand(Controllers* controllers, QObject* parent)
	: d(std::make_unique<Private>(this))
	, ICommand(parent) {
	d->controllers = controllers;
}

WindowsCommand::~WindowsCommand() = default;

QString WindowsCommand::help() const {
	return R"(windows action=<command>[, param=<value>]
	close id=<id | active | all>
	list
	create target=<window type>[, id=<id>][, title=<title>]
	invoke id=<id | active>, cmd=<window command>
		Example:
		windows action=invoke, cmd="style word-wrap=true"
	)";
}

bool WindowsCommand::execute(std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto action = context->variable("action").toString();
	if (action.isEmpty()) {
		controller->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "close") {
		return WindowsNs::handleClose(context->variable("id").toString(), context, d->controllers);
	}
	else if (action == "create") {
		return WindowsNs::handleCreate(context, d->controllers);
	}
	else if (action == "list") {
		return WindowsNs::handleList(context, d->controllers);
	}
	else if (action == "invoke") {
		return WindowsNs::handleInvoke(context, d->controllers);
	}

	controller->printError(QString("Usage: %1").arg(help()));
	return false;
}
