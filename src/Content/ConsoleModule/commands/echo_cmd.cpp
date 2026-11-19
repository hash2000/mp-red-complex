#include "Content/ConsoleModule/commands/echo_cmd.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/console/console_table.h"

#include <QDomDocument>
#include <QJsonDocument>

QString EchoCommand::help() const {
	return "echo name=<text>[, name2=<text2>] print all params and text";
}

bool EchoCommand::execute(std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();

	ConsoleTable table({ "Name", "mime-type", "text" });

	const auto parameters = context->ownVariables();
	for (auto it = parameters.cbegin(); it != parameters.cend(); it++) {
		const auto mimeType = context->variableMimeType(it.key());
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

	controller->print(table);
	return true;
}
