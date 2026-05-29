#include "Game/commands/cmd/window_invoke_cmd.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/controllers.h"
#include "Game/controllers/windows_controller.h"
#include "Game/app_controller.h"

#include <QRegularExpression>
#include <QStringList>

bool WindowInvokeCommand::execute(CommandContext* context, const QStringList& args) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();

	const auto cmd = args.filter(QRegularExpression("^cmd:")).value(0).mid(4);
	if (cmd.isEmpty()) {
		context->printError(QString("Need parameter 'cmd'. Usage: %1").arg(help()));
		return false;
	}

	MdiChildWindow* targetEntry;

	const auto target = args.filter(QRegularExpression("^target:")).value(0).mid(7);
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
