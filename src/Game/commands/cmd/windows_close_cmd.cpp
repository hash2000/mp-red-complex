#include "Game/commands/cmd/windows_close_cmd.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/app_controller.h"
#include "Game/controllers/windows_controller.h"

bool CloseWindowCommand::execute(CommandContext* context, const QStringList& args) {
	if (args.isEmpty()) {
		context->printError("Usage: close <window_id | 'active' | 'all'>");
		return false;
	}

	QString target = args.first().toLower();
	auto controller = context->applicationController()->windowsController();

	if (target == "active") {
		auto activeEntry = controller->activeWindowEntry();
		if (!activeEntry.first) {
			context->printError("No active window");
			return false;
		}

		QString windowId = activeEntry.second;
		controller->closeWindowById(windowId);
		context->printSuccess(QString("Closed active window [%1]")
			.arg(windowId));
		return true;

	}
	else if (target == "all") {
		int count = controller->closeAllWindows();
		context->printSuccess(QString("Closed %1 window(s)").arg(count));
		return true;

	}
	else {
		if (controller->findWindowById(target)) {
			controller->closeWindowById(target);
			context->printSuccess(QString("Closed window [%1]").arg(target));
			return true;
		}
		else {
			context->printError(QString("Window not found: %1").arg(target));
			return false;
		}
	}
}
