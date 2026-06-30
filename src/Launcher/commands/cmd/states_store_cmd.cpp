#include "Launcher/commands/cmd/states_store_cmd.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/instruction.h"
#include "Launcher/services.h"

bool StatesStoreCommand::execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	auto services = context->services();
	const auto command = instruction->command.toLower();

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

	context->printError(QString("Usage: %1")
		.arg(help()));
	return false;
}
