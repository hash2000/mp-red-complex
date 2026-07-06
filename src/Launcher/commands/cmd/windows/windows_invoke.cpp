#include "Launcher/commands/cmd/windows/windows_invoke.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/windows_builder.h"
#include "Launcher/commands/instruction.h"

namespace WindowsNs {
bool handleInvoke(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	auto controller = context->controllers()->windowsController();
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
