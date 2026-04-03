#include "Game/widgets/user/login_window.h"
#include "Game/widgets/user/login_widget.h"
#include "Game/app_controller.h"
#include "Game/commands/command_context.h"
#include "Game/controllers.h"
#include "Game/widgets/action_panel/action_panel_by_user_builder.h"
#include "Game/controllers/action_panel_controller.h"
#include "DataLayer/users/user.h"
#include "ApplicationLayer/users/users_service.h"
#include <QMessageBox>

class LoginWindow::Private {
public:
	Private(LoginWindow* parent)
		: q(parent) {
	}

	LoginWindow* q;
	UsersService* usersService;
	LoginWidget* loginWidget = nullptr;
	ApplicationController* controller = nullptr;
};

LoginWindow::LoginWindow(UsersService* usersService, const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
	d->usersService = usersService;

	// Создаём виджет входа
	d->loginWidget = new LoginWidget(usersService, this);

	// Подключаем сигналы
	connect(d->loginWidget, &LoginWidget::loginSuccess, this, &LoginWindow::onLoginSuccess);
	connect(d->loginWidget, &LoginWidget::registerSuccess, this, &LoginWindow::onRegisterSuccess);

	setWidget(d->loginWidget);
}

LoginWindow::~LoginWindow() = default;

bool LoginWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		d->controller = context->applicationController();
		return true;
	}

	return false;
}

void LoginWindow::onLoginSuccess() {
	// Получаем данные текущего пользователя
	auto userOpt = d->usersService->currentUser();
	if (userOpt.has_value()) {
		close();
	}
}

void LoginWindow::onRegisterSuccess() {

}
