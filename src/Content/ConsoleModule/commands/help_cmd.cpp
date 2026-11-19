#include "Content/ConsoleModule/commands/help_cmd.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/console/console_table.h"
#include "Libs/Resources/resources.h"
#include "Libs/Resources/variables/variables_context.h"

bool HelpCommand::execute(std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto processor = controller->commandProcessor();

	const auto parameters = context->ownVariables();
	if (parameters.isEmpty()) {
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

	// Предмет справки — значение первого параметра: `help name=windows`.
	const auto subject = parameters.cbegin().value().toString();
	if (subject.isEmpty()) {
		return false;
	}

	const auto command = processor->findCommand(subject);
	if (command) {
		ConsoleTable commandsTab({ "Name", "Descroption" });
		commandsTab.addRow({ command->name(), command->description() });
		controller->print(commandsTab);
		return true;
	}

	const auto& variables = controller->resources()->Variables;
	const auto variable = variables.get(subject, QVariant());
	if (variable.isValid()) {
		ConsoleTable variablesTab({ "Name", "Value" });
		variablesTab.addRow({ subject, variable.toString() });
		controller->print(variablesTab);
		return true;
	}

	return false;
}
