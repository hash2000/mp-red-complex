#include "Content/ConsoleModule/commands/echo_cmd.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/i_command.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_json.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Resources/resources.h"
#include "Libs/Resources/variables/variables_context.h"

#include <QDomDocument>
#include <QJsonDocument>

QString EchoCommand::help() const {
		return "echo {param_name}:{text} print all params and text";
}

bool EchoCommand::execute(const std::shared_ptr<Instruction> instruction, CommandContext* context) {
	const auto processor = context->commandController()->commandProcessor();

	ConsoleTable table({ "Name", "mime-type", "text" });

	for (auto it = instruction->parameters.begin(); it != instruction->parameters.end(); it++) {
		const auto mimeType = instruction->mimetype(it.key());
		QString text;

		if (mimeType == "application/xml") {
			const auto value = it.value().value<QDomDocument>();
			text = value.toString();
		}
		else if (mimeType == "text/x-ini") {
			const auto values = it.value().value<QMap<QString, QVariant>>();
			for (auto i = values.begin(); i != values.end(); i++) {
				text += QString("%1 = %2\r\n")
					.arg(i.key())
					.arg(i.value().toString());
			}
		}
		else if (mimeType == "application/json") {
			text = QString::fromUtf8(it.value().toJsonDocument().toJson(QJsonDocument::Indented));
		}
		else {
			text = it.value().toString();
		}

		table.addRow({
			it.key(),
			mimeType,
			text
			});
	}

	context->print(table);
	return true;
}
