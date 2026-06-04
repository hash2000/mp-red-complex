#include "Launcher/commands/cmd/help_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/app_controller.h"

bool HelpCommand::execute(CommandContext* context, const QStringList& args) {
	if (args.isEmpty()) {
		context->print(context->applicationController()->commandProcessor()->fullHelp());
	}
	else {
		QString cmdName = args.first();
		context->print(context->applicationController()->commandProcessor()->helpForCommand(cmdName));
	}
	return true;
}
