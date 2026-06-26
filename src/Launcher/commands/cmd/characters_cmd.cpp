#include "Launcher/commands/cmd/characters_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/command_console/console_table.h"
#include "Launcher/commands/command_console/console_image.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"

#include "Content/CharactersModule/services/characters_service.h"
#include "Content/CharactersModule/models/character.h"

#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/models/user_view.h"


class CharactersCommand::Private {
public:
	Private(CharactersCommand* parent) : q(parent) {}
	CharactersCommand* q;

	void printCharacter(ConsoleTable& table, const std::shared_ptr<Character> chr);
	bool showCharacters(CommandContext* context, const QString& userId);
	bool showCharacter(CommandContext* context, const QUuid& id);
};


CharactersCommand::CharactersCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, CommandAbstraction(parent) {}

CharactersCommand::~CharactersCommand() = default;

QString CharactersCommand::help() const {
	return R"(characters action:
	show-user id:{user_id}
	show-character id:{character_id}
	)";
}

void CharactersCommand::Private::printCharacter(ConsoleTable& table, const std::shared_ptr<Character> chr) {
	table.addRow({
		chr->icon,
		chr->id.toString(QUuid::WithoutBraces),
		chr->name,
		chr->level,
		chr->equipmentId.toString(QUuid::WithoutBraces)
		});
}

bool CharactersCommand::Private::showCharacters(CommandContext* context, const QString& userId) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto charactersService = services->charactersService();
	auto chrs = charactersService->charactersByUser(userId);

	ConsoleTable table({ "..", "Id", "Name", "Lvl", "EquipmentId" });

	for (const auto chr : chrs) {
		printCharacter(table, chr);
	}

	context->print(table);
	return true;
}

bool CharactersCommand::Private::showCharacter(CommandContext* context, const QUuid& id) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto charactersService = services->charactersService();
	auto chr = charactersService->character(id);
	if (!chr) {
		context->printError("Undefined user");
		return false;
	}

	ConsoleTable table({ "..", "Id", "Name", "Lvl", "EquipmentId" });
	printCharacter(table, chr);
	context->print(table);
	return true;
}

bool CharactersCommand::execute(CommandContext* context, const QStringList& args) {
	const auto action = parseArgsValue(args, "action");
	if (action.isEmpty()) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "show-user") return d->showCharacters(context, parseArgsValue(args, "id"));
	else if (action == "show-character") return d->showCharacter(context, QUuid::fromString(parseArgsValue(args, "id")));

	return true;
}


