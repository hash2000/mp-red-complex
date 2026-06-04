#include "Launcher/commands/cmd/states_store_cmd.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/services.h"

bool StatesStoreCommand::execute(CommandContext* context, const QStringList& args) {
	auto services = context->services();
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

	context->printError(QString("Usage: %1").arg(help()));
	return false;
}
