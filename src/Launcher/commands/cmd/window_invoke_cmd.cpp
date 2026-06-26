#include "Launcher/commands/cmd/window_invoke_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/controllers.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/app_controller.h"

#include <QStringList>

QString WindowInvokeCommand::help() const {
	return R"(
		window-invoke target:window[or active window] cmd:command [parameters that depend on the command being called]
		Example:
		window-invoke cmd:style word-wrap:true font:Sans
)";
}


bool WindowInvokeCommand::execute(CommandContext* context, const QStringList& args) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();

	const auto cmd = parseArgsValue(args, "cmd");
	if (cmd.isEmpty()) {
		context->printError(QString("Need parameter 'cmd'. Usage: %1").arg(help()));
		return false;
	}

	MdiChildWindow* targetEntry;

	const auto target = parseArgsValue(args, "target");
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

	if (!targetEntry->handleCommand(cmd, args, context)) {
		context->printError(QString("Method '%1' returned false. target  title '%2'")
			.arg(cmd)
			.arg(target));
		return false;
	}

	return true;
}
