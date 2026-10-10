#include "Launcher/commands/windows/windows_close.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_image.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Engine/services/services_registry.h"

#include "Libs/Engine/mdi_child_window.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"

namespace WindowsNs {

bool handleCloseAll(std::shared_ptr<CommandContext> context, Controllers* controllers) {
	const auto wndController = controllers->windowsController();
	const auto cmdController = context->commandController();
	auto windows = wndController->allWindows();
	int count = windows.size();
	for (MdiChildWindow* window : windows) {
		window->deleteLater();
	}

	cmdController->printSuccess(QString("Closed %1 window(s)")
		.arg(count));

	return true;
}

bool handleCloseActive(std::shared_ptr<CommandContext> context, Controllers* controllers) {
	const auto wndController = controllers->windowsController();
	const auto cmdController = context->commandController();
	auto activeEntry = wndController->activeWindowEntry();
	if (!activeEntry.first) {
		cmdController->printError("No active window");
		return false;
	}

	const auto windowId = activeEntry.second;
	wndController->closeWindowById(windowId);
	cmdController->printSuccess(QString("Closed active window [%1]")
		.arg(windowId));

	return true;
}

bool handleCloseById(const QString& id, std::shared_ptr<CommandContext> context, Controllers* controllers) {
	const auto wndController = controllers->windowsController();
	const auto cmdController = context->commandController();
	if (wndController->findWindowById(id)) {
		wndController->closeWindowById(id);
		cmdController->printSuccess(QString("Closed window [%1]")
			.arg(id));
		return true;
	}

	cmdController->printError(QString("Window not found: %1")
		.arg(id));

	return false;
}

bool handleClose(const QString& id, std::shared_ptr<CommandContext> context, Controllers* controllers) {

	if (id == "all") {
		return handleCloseAll(context, controllers);
	}
	else if (id == "active") {
		return handleCloseActive(context, controllers);
	}

	return handleCloseById(id, context, controllers);
}
}
