#include "Launcher/commands/windows/windows_invoke.h"
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
bool handleInvoke(const std::shared_ptr<Instruction> instruction, CommandContext* context, Controllers* controllers) {
	auto controller = controllers->windowsController();
	auto services = context->services();

	const auto cmd = instruction->text("cmd");
	if (cmd.isEmpty()) {
		context->printError(QString("Need parameter 'cmd'."));
		return false;
	}

	MdiChildWindow* targetEntry;

	const auto target = instruction->text("target");
	if (target.isEmpty()) {
		auto activeEntry = controller->activeWindowEntry();
		targetEntry = activeEntry.first.data();
	}
	else {
		targetEntry = controller->findWindowById(target);
	}

	if (!targetEntry) {
		context->printError("Unknown target window");
		return false;
	}

	if (!targetEntry->handleCommand(instruction, context)) {
		context->printError(QString("Method '%1' returned false. target  title '%2'")
			.arg(cmd)
			.arg(target));
		return false;
	}

	return true;
}
}
