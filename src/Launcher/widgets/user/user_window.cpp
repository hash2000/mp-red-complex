#include "Launcher/widgets/user/user_window.h"
#include "Launcher/widgets/user/user_widget.h"
#include "Launcher/app_controller.h"
#include "Launcher/services.h"
#include "Launcher/app_controller.h"
#include "Launcher/commands/command_context.h"
#include "ApplicationLayer/users/users_service.h"
#include "ApplicationLayer/character/character_item_handler.h"

class UserWindow::Private {
public:
	Private(UserWindow* parent) : q(parent) { }
	UserWindow* q;

	UsersService* usersService = nullptr;
	ImagesService* imagesService = nullptr;
	UserWidget* userWidget = nullptr;
	ApplicationController* applicationController = nullptr;

	void executeCharCommand(const QString& target, const CharacterItemHandler* chr);
};

UserWindow::UserWindow(const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
}

UserWindow::~UserWindow() = default;

void UserWindow::Private::executeCharCommand(const QString& target, const CharacterItemHandler* chr) {
	const auto itemIdStr = QString("id:%1")
		.arg(chr->equipmentId
			.toString(QUuid::StringFormat::WithoutBraces)
			.toLower());

	const auto title = QString("title:%1")
		.arg(chr->name);

	const auto fullTarget = QString("target:%1")
		.arg(target);

	applicationController->executeCommandByName("window-create", QStringList{
		fullTarget, itemIdStr, title });
}

bool UserWindow::handleCommand(const QString& commandName, const QStringList& args, CommandContext* context) {
	if (commandName == "create") {
		auto services = context->services();
		d->applicationController = context->applicationController();
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

	d->executeCharCommand("equipment", chr);
}

void UserWindow::onSpecificationsRequested(const QUuid& characterId) {
	const auto chr = d->usersService->getCharacter(characterId);
	if (!chr) {
		return;
	}

	d->executeCharCommand("character-specifications", chr);
}

void UserWindow::onUserLoggedOut() {
	close();
}
