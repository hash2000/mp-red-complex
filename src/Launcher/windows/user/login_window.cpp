#include "Launcher/windows/user/login_window.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Content/UsersModule/widgets/login_widget.h"
#include "Content/UsersModule/models/user_view.h"
#include "Content/UsersModule/services/users_service.h"
#include "Libs/Engine/services/services_registry.h"

#include <QMessageBox>

class LoginWindow::Private {
public:
	Private(LoginWindow* parent) : q(parent) { }
	LoginWindow* q;

	UsersService* usersService = nullptr;
	LoginWidget* loginWidget = nullptr;
};

LoginWindow::LoginWindow(const QString& id, std::shared_ptr<CommandContext> context, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, context, parent) {
}

LoginWindow::~LoginWindow() = default;

bool LoginWindow::handleCommand(const std::shared_ptr<Instruction> cmd) {
	const auto action = cmd->parameters.value("action");
	if (action == "create") {
		const auto context = currentContext();
		const auto services = context->commandController()->services();
		d->usersService = services->get<UsersService>();
		d->loginWidget = new LoginWidget(d->usersService, this);

		connect(d->loginWidget, &LoginWidget::loginSuccess, this, &LoginWindow::onLoginSuccess);
		connect(d->loginWidget, &LoginWidget::registerSuccess, this, &LoginWindow::onRegisterSuccess);

		setWidget(d->loginWidget);

		return true;
	}

	return false;
}

void LoginWindow::onLoginSuccess() {
	//auto userOpt = d->usersService->currentUser();
	//if (userOpt) {
	//	close();
	//}
}

void LoginWindow::onRegisterSuccess() {

}
