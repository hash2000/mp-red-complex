#include "Content/ConsoleModule/commands/help_cmd.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/i_command.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Resources/resources.h"
#include "Libs/Resources/variables/variables_context.h"

bool HelpCommand::execute(const std::shared_ptr<Instruction> instruction, std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto processor = controller->commandProcessor();

	if (instruction->parameters.isEmpty()) {
		controller->print("Commands:");
		ConsoleTable commandsTab({ "Name", "Descroption", "Help" });
		for (const auto& name : processor->availableCommands()) {
			const auto cmd = processor->findCommand(name);
			commandsTab.addRow({ cmd->name(), cmd->description(), cmd->help() });
		}
		controller->print(commandsTab);

		controller->print("Variables:");
		const auto& variables = controller->resources()->Variables;
		const auto& variablesNames = variables.available();
		ConsoleTable variablesTab({ "Name", "Value" });
		for (const auto& name : variablesNames) {
			const auto var = variables.get(name, QVariant());
			variablesTab.addRow({ name, var.toString() });
		}
		controller->print(variablesTab);
		return true;
	}
	else {
		QString cmdName = instruction->command;
		const auto cmd = processor->findCommand(cmdName);
		if (cmd) {
			ConsoleTable commandsTab({ "Name", "Descroption" });
			commandsTab.addRow({ cmd->name(), cmd->description() });
			controller->print(commandsTab);
			return true;
		}

		const auto& variables = controller->resources()->Variables;
		const auto var = variables.get(cmdName, QVariant());
		if (var.isValid()) {
			ConsoleTable variablesTab({ "Name", "Value", "Help" });
			variablesTab.addRow({ cmdName, var.toString(), cmd->help() });
			controller->print(variablesTab);
			return true;
		}
	}

	return false;
}
