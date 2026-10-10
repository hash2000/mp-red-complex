#include "Launcher/commands/windows/windows_invoke.h"
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
bool handleInvoke(const std::shared_ptr<Instruction> instruction, std::shared_ptr<CommandContext> context, Controllers* controllers) {
	const auto wndController = controllers->windowsController();
	const auto cmdController = context->commandController();
	const auto services = cmdController->services();

	const auto cmd = instruction->text("cmd");
	if (cmd.isEmpty()) {
		cmdController->printError(QString("Need parameter 'cmd'."));
		return false;
	}

	MdiChildWindow* targetEntry;

	const auto target = instruction->text("target");
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

	if (!targetEntry->handleCommand(instruction)) {
		cmdController->printError(QString("Method '%1' returned false. target  title '%2'")
			.arg(cmd)
			.arg(target));
		return false;
	}

	return true;
}
}
