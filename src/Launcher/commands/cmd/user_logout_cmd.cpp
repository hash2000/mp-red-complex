#include "Launcher/commands/cmd/user_logout_cmd.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/services.h"
#include "ApplicationLayer/users/users_service.h"

bool UserLogoutCommand::execute(CommandContext* context, const QStringList& args) {
	auto service = context->services()->usersService();
	if (service->isAuthenticated()) {
		service->logout();
		context->print("User logout");
	}
	else {
		context->printError("User is not authorized");
	}

	return true;
}
