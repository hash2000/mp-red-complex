#include "Content/UsersModule/commands/users_cmd.h"
#include "Content/ConsoleModule/processors/command_processor.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/command_console/console_table.h"
#include "Content/ConsoleModule/command_console/console_image.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Libs/Engine/services/services_registry.h"

#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/services/users_registry_service.h"
#include "Content/UsersModule/models/user_view.h"

class UsersCommand::Private {
public:
	Private(UsersCommand* parent) : q(parent) { }
	UsersCommand* q;

	void printUser(ConsoleTable& table, const std::shared_ptr<UserView> user);
	void printUsersFullInfo(std::shared_ptr<CommandContext> context, const std::list<std::shared_ptr<UserView>>& users);
	bool login(std::shared_ptr<CommandContext> context, const QString& username, const QString& password);
	bool logout(std::shared_ptr<CommandContext> context);
	bool registerUser(std::shared_ptr<CommandContext> context, const QString& username, const QString& password);
	bool restoreAccount(std::shared_ptr<CommandContext> context, const QString& seed, const QString& password);
};


UsersCommand::UsersCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, ICommand(parent)
{
}

UsersCommand::~UsersCommand() = default;

QString UsersCommand::help() const {
	return R"(users action:
	logout
	login username:{login} password:{password}
	register: username:{login} password:{password}
	restore: seed:{list of seed phrase, all words are separated by a space} password:{new password}
	)";
}

void UsersCommand::Private::printUser(ConsoleTable& table, const std::shared_ptr<UserView> user) {
	QString permissions;
	for (auto it = user->permissions.begin(); it != user->permissions.end(); it++) {
		if (it != user->permissions.begin()) {
			permissions += "<br/>";
		}
		permissions += *it;
	}

	table.addRow({
		user->data->icon,
		user->data->loginHash,
		user->data->displayName,
		user->data->createdAt.toString("yyyy-MM-dd HH:mm:ss"),
		permissions
		});
}

void UsersCommand::Private::printUsersFullInfo(std::shared_ptr<CommandContext> context, const std::list<std::shared_ptr<UserView>>& users) {
	const auto controller = context->commandController();

	ConsoleTable table({ "..", "Id", "Name", "Created", "Permissions" });
	for (const auto user : users) {
		printUser(table, user);
	}

	controller->print(table);
}

bool UsersCommand::Private::login(std::shared_ptr<CommandContext> context, const QString& username, const QString& password) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto usersService = services->get<UsersService>();

	auto loginResult = usersService->login(username, password);
	if (!loginResult) {
		controller->printError("User login failed");
		return false;
	}

	controller->printSuccess("User login");
	return true;
}

bool UsersCommand::Private::logout(std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto usersService = services->get<UsersService>();

	if (!usersService->isAuthenticated()) {
		controller->printError("User is not authorized");
		return false;
	}

	usersService->logout();
	controller->printSuccess("User logout");
	return true;
}

bool UsersCommand::Private::registerUser(std::shared_ptr<CommandContext> context, const QString& username, const QString& password) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto registryService = services->get<UsersRegistryService>();

	QStringList seedPhrase;
	if (!registryService->registerUser(username, password, seedPhrase)) {
		controller->printError(QString("Can't register user %1")
			.arg(username));
		return false;
	}

	controller->printSuccess(QString("User registered. %1. Seed phrase: [%2]")
		.arg(username)
		.arg(seedPhrase.join(" ")));
	return true;
}

bool UsersCommand::Private::restoreAccount(std::shared_ptr<CommandContext> context, const QString& seed, const QString& password) {
	const auto controller = context->commandController();
	const auto services = controller->services();
	auto registryService = services->get<UsersRegistryService>();
	const auto wordList = seed.split(" ", Qt::SkipEmptyParts);
	if (wordList.size() != 12 || wordList.size() != 24) {
		controller->printError("Seed prase is wrong.");
		return false;
	}

	controller->printSuccess("User restored.");
	return true;
}

bool UsersCommand::execute(const std::shared_ptr<Instruction> instruction, std::shared_ptr<CommandContext> context) {
	const auto controller = context->commandController();
	const auto action = instruction->text("action");
	if (action.isEmpty()) {
		controller->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "login") {
		return d->login(context,
			instruction->text("username"),
			instruction->text("password"));
	}
	else if (action == "logout") {
		return d->logout(context);
	}
	else if (action == "register") {
		return d->registerUser(context,
			instruction->text("username"),
			instruction->text("password"));
	}
	else if (action == "restore") {
		return d->restoreAccount(context,
			instruction->text("seed"),
			instruction->text("password"));
	}

	return true;
}
