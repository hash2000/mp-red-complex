#include "Game/commands/cmd/states_store_cmd.h"
#include "Game/commands/command_context.h"
#include "Game/app_controller.h"
#include "Game/controllers/windows_controller.h"
#include "Game/services.h"

bool StatesStoreCommand::execute(CommandContext* context, const QStringList& args) {
	auto services = context->applicationController()->services();
	const auto command = args.first().toLower();

	if (command == "save") {
		context->print("Save states");
		services->postSaveEvent();
		return true;
	}

	if (command == "load") {
		context->print("Load states");
		services->postLoadEvent();
		return true;
	}

	return false;
}
