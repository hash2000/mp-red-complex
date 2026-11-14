#include "Launcher/commands/windows/windows_invoke.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Engine/console/console_image.h"
#include "Libs/Engine/services/services_registry.h"

#include "Libs/Engine/mdi_child_window.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"

namespace WindowsNs {
bool handleInvoke(std::shared_ptr<CommandContext> context, Controllers* controllers) {
	const auto wndController = controllers->windowsController();
	const auto cmdController = context->commandController();

	const auto cmd = context->variable("cmd").toString();
	if (cmd.isEmpty()) {
		cmdController->printError(QString("Need parameter 'cmd'."));
		return false;
	}

	MdiChildWindow* targetEntry;

	const auto target = context->variable("target").toString();
	if (target.isEmpty()) {
		auto activeEntry = wndController->activeWindowEntry();
		targetEntry = activeEntry.first.data();
	}
	else {
		targetEntry = wndController->findWindowById(target);
	}

	if (!targetEntry) {
		cmdController->printError("Unknown target window");
		return false;
	}

	if (!targetEntry->handleCommand(context)) {
		cmdController->printError(QString("Method '%1' returned false. target  title '%2'")
			.arg(cmd)
			.arg(target));
		return false;
	}

	return true;
}
}
