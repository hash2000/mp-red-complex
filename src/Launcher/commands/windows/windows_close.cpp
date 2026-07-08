#include "Launcher/commands/windows/windows_close.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_image.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Engine/services/services_registry.h"

#include "Launcher/mdi_child_window.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"

namespace WindowsNs {

bool handleCloseAll(CommandContext* context, Controllers* controllers) {
	auto controller = controllers->windowsController();
	auto windows = controller->allWindows();
	int count = windows.size();
	for (MdiChildWindow* window : windows) {
		window->deleteLater();
	}

	context->printSuccess(QString("Closed %1 window(s)")
		.arg(count));

	return true;
}

bool handleCloseActive(CommandContext* context, Controllers* controllers) {
	auto controller = controllers->windowsController();
	auto activeEntry = controller->activeWindowEntry();
	if (!activeEntry.first) {
		context->printError("No active window");
		return false;
	}

	const auto windowId = activeEntry.second;
	controller->closeWindowById(windowId);
	context->printSuccess(QString("Closed active window [%1]")
		.arg(windowId));

	return true;
}

bool handleCloseById(const QString& id, CommandContext* context, Controllers* controllers) {
	auto controller = controllers->windowsController();
	if (controller->findWindowById(id)) {
		controller->closeWindowById(id);
		context->printSuccess(QString("Closed window [%1]")
			.arg(id));
		return true;
	}

	context->printError(QString("Window not found: %1")
		.arg(id));

	return false;
}

bool handleClose(const QString& id, CommandContext* context, Controllers* controllers) {
	if (id == "all") return handleCloseAll(context, controllers);
	else if (id == "active") return handleCloseActive(context, controllers);

	return handleCloseById(id, context, controllers);
}
}
