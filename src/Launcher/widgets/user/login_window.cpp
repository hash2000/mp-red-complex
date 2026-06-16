#include "Launcher/widgets/user/login_window.h"
#include "Launcher/app_controller.h"
#include "Launcher/commands/command_context.h"
#include "Launcher/controllers.h"
#include "Launcher/services.h"
#include "Launcher/widgets/action_panel/action_panel_by_user_builder.h"
#include "Launcher/controllers/action_panel_controller.h"
#include "Content/UsersModule/widgets/login_widget.h"
#include "Content/UsersModule/models/user.h"
#include "Content/UsersModule/services/users_service.h"
#include <QMessageBox>

class LoginWindow::Private {
public:
	Private(LoginWindow* parent) : q(parent) { }
	LoginWindow* q;

	UsersService* usersService = nullptr;
	LoginWidget* loginWidget = nullptr;
	ApplicationController* controller = nullptr;
};

LoginWindow::LoginWindow(const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
}

LoginWindow::~LoginWindow() = default;

bool LoginWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		auto services = context->services();
		d->usersService = services->usersService();
		d->loginWidget = new LoginWidget(d->usersService, this);

		connect(d->loginWidget, &LoginWidget::loginSuccess, this, &LoginWindow::onLoginSuccess);
		connect(d->loginWidget, &LoginWidget::registerSuccess, this, &LoginWindow::onRegisterSuccess);

		setWidget(d->loginWidget);

		return true;
	}

	return false;
}

void LoginWindow::onLoginSuccess() {
	// Получаем данные текущего пользователя
	auto userOpt = d->usersService->currentUser();
	if (userOpt) {
		close();
	}
}

void LoginWindow::onRegisterSuccess() {

}
