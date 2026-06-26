#include "Launcher/commands/cmd/users_cmd.h"
#include "Launcher/commands/command_processor.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/commands/command_console/console_table.h"
#include "Launcher/commands/command_console/console_image.h"
#include "Launcher/controllers/windows_controller.h"
#include "Launcher/app_controller.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"

#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/models/user_view.h"

class UsersCommand::Private {
public:
	Private(UsersCommand* parent) : q(parent) { }
	UsersCommand* q;

	void printUser(ConsoleTable& table, const std::shared_ptr<UserView> user);
	void printUsersFullInfo(CommandContext* context, const std::list<std::shared_ptr<UserView>>& users);
	bool login(CommandContext* context, const QString& login, const QString& password);
	bool logout(CommandContext* context);
	bool registerInTarget(CommandContext* context, const QString& login, const QString& target);
};


UsersCommand::UsersCommand(QObject* parent)
	: d(std::make_unique<Private>(this))
	, CommandAbstraction(parent)
{
}

UsersCommand::~UsersCommand() = default;

QString UsersCommand::help() const {
	return R"(users action:
	logout
	login login:{login} password:{password}
	)";
}

void UsersCommand::Private::printUser(ConsoleTable& table, const std::shared_ptr<UserView> user) {
	QString permissions;
	for (auto it = user->permissions.begin(); it != user->permissions.end(); it++) {
		if (it != user->permissions.begin()) permissions += "<br/>";
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

void UsersCommand::Private::printUsersFullInfo(CommandContext* context, const std::list<std::shared_ptr<UserView>>& users) {
	ConsoleTable table({ "..", "Id", "Name", "Created", "Permissions" });

	for (const auto user : users) {
		printUser(table, user);
	}

	context->print(table);
}

bool UsersCommand::Private::login(CommandContext* context, const QString& login, const QString& password) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto usersService = services->usersService();

	auto loginResult = usersService->login(login, password);
	if (!loginResult) {
		context->printError("User login failed");
		return false;
	}

	auto user = usersService->currentUser();

	printUsersFullInfo(context, { user });

	context->printSuccess("User login");
	return true;
}

bool UsersCommand::Private::logout(CommandContext* context) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto usersService = services->usersService();

	if (!usersService->isAuthenticated()) {
		context->printError("User is not authorized");
		return false;
	}

	usersService->logout();
	context->printSuccess("User logout");
	return true;
}

bool UsersCommand::Private::registerInTarget(CommandContext* context, const QString& login, const QString& target) {
	auto controller = context->controllers()->windowsController();
	auto services = context->services();
	auto usersService = services->usersService();

	if (!usersService->isAuthenticated()) {
		context->printError("User is not authorized");
		return false;
	}
}

bool UsersCommand::execute(CommandContext* context, const QStringList& args) {
	const auto action = parseArgsValue(args, "action");
	if (action.isEmpty()) {
		context->printError(QString("Usage: %1").arg(help()));
		return false;
	}

	if (action == "login") return d->login(context,
		parseArgsValue(args, "login"),
		parseArgsValue(args, "password"));
	else if (action == "logout") return d->logout(context);

	return true;
}
