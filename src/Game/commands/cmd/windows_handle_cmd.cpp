#include "Game/commands/cmd/windows_handle_cmd.h"
#include "Game/commands/command_processor.h"
#include "Game/commands/command_context.h"
#include "Game/app_controller.h"
#include "Game/controllers/windows_controller.h"

bool HandleWindowsCommand::execute(CommandContext* context, const QStringList& args) {
	Q_UNUSED(args);

	auto controller = context->applicationController();
	auto entries = controller->windowsController()->windowEntries();


	return true;
}
