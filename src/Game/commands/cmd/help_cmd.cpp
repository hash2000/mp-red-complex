#include "Game/commands/cmd/help_cmd.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/app_controller.h"

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
