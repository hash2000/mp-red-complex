#include "Launcher/commands/cmd/help_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/command_console/console_table.h"
#include "Launcher/app_controller.h"
#include "Libs/Resources/resources.h"
#include "Libs/Resources/variables/variables_context.h"

bool HelpCommand::execute(CommandContext* context, const QStringList& args) {
	const auto processor = context->applicationController()->commandProcessor();

	if (args.isEmpty()) {
		context->print("Commands:");
		ConsoleTable commandsTab({ "Name", "Descroption", "Help" });
		for (const auto& name : processor->availableCommands()) {
			const auto cmd = processor->findCommand(name);
			commandsTab.addRow({ cmd->name(), cmd->description(), cmd->help() });
		}
		context->print(commandsTab);

		context->print("Variables:");
		const auto& variables = context->applicationController()->resources()->Variables;
		const auto& variablesNames = variables.available();
		ConsoleTable variablesTab({ "Name", "Value" });
		for (const auto& name : variablesNames) {
			const auto var = variables.get(name, QVariant());
			variablesTab.addRow({ name, var.toString() });
		}
		context->print(variablesTab);
		return true;
	}
	else {
		QString cmdName = args.first();
		const auto cmd = processor->findCommand(cmdName);
		if (cmd) {
			ConsoleTable commandsTab({ "Name", "Descroption" });
			commandsTab.addRow({ cmd->name(), cmd->description() });
			context->print(commandsTab);
			return true;
		}

		const auto& variables = context->applicationController()->resources()->Variables;
		const auto var = variables.get(cmdName, QVariant());
		if (var.isValid()) {
			ConsoleTable variablesTab({ "Name", "Value", "Help" });
			variablesTab.addRow({ cmdName, var.toString(), cmd->help() });
			context->print(variablesTab);
			return true;
		}
	}

	return false;
}
