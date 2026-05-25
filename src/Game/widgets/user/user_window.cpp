#include "Game/widgets/user/user_window.h"
#include "Game/widgets/user/user_widget.h"
#include "Game/app_controller.h"
#include "Game/services.h"
#include "Game/commands/command_context.h"
#include "ApplicationLayer/users/users_service.h"
#include "ApplicationLayer/character/character_item_handler.h"

class UserWindow::Private {
public:
	Private(UserWindow* parent) : q(parent) { }
	UserWindow* q;

	UsersService* usersService = nullptr;
	ImagesService* imagesService = nullptr;
	UserWidget* userWidget = nullptr;
	ApplicationController* controller = nullptr;
};

UserWindow::UserWindow(const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
}

UserWindow::~UserWindow() = default;

bool UserWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		d->controller = context->applicationController();
		auto services = d->controller->services();
		d->usersService = services->usersService();
		d->imagesService = services->imagesService();
		d->userWidget = new UserWidget(d->usersService, d->imagesService, this);

		// Подключаем сигналы
		connect(d->userWidget, &UserWidget::equipmentRequested, this, &UserWindow::onEquipmentRequested);
		connect(d->userWidget, &UserWidget::specificationsRequested, this, &UserWindow::onSpecificationsRequested);
		connect(d->usersService, &UsersService::loggedOut, this, &UserWindow::onUserLoggedOut);

		setWidget(d->userWidget);

		return true;
	}
	return false;
}

void UserWindow::onEquipmentRequested(const QUuid& characterId) {
	const auto chr = d->usersService->getCharacter(characterId);
	if (!chr) {
		return;
	}

	d->controller->executeCommandByName("window-create", QStringList{
		"equipment", chr->equipmentId.toString(QUuid::StringFormat::WithoutBraces), chr->name });
}

void UserWindow::onSpecificationsRequested(const QUuid& characterId) {
	const auto chr = d->usersService->getCharacter(characterId);
	if (!chr) {
		return;
	}

	d->controller->executeCommandByName("window-create", QStringList{
		"character-specifications", chr->id.toString(QUuid::StringFormat::WithoutBraces), chr->name });
}

void UserWindow::onUserLoggedOut() {
	close();
}
