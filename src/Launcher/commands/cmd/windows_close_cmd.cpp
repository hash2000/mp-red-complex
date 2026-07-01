#include "Launcher/commands/cmd/windows_close_cmd.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/instruction.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"

bool CloseWindowsCommand::execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	if (instruction->parameters.isEmpty()) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	const auto target = instruction->text("target");
	auto controller = context->controllers()->windowsController();

	if (target == "active") {
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
