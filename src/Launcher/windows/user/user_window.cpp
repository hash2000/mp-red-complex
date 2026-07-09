#include "Launcher/windows/user/user_window.h"
#include "Content/ConsoleModule/command_context.h"
#include "Content/ConsoleModule/models/instruction.h"
#include "Content/UsersModule/widgets/user_widget.h"
#include "Content/UsersModule/services/users_service.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Libs/Engine/services/services_registry.h"

class UserWindow::Private {
public:
	Private(UserWindow* parent) : q(parent) { }
	UserWindow* q;

	UsersService* usersService = nullptr;
	ImagesService* imagesService = nullptr;
	UserWidget* userWidget = nullptr;
	CommandController* commandController = nullptr;

//	void executeCharCommand(const QString& target, const Character* chr);
};

UserWindow::UserWindow(const QString& id, QWidget* parent)
	: d(std::make_unique<Private>(this))
	, MdiChildWindow(id, parent) {
}

UserWindow::~UserWindow() = default;

//void UserWindow::Private::executeCharCommand(const QString& target, const Character* chr) {
//	const auto itemIdStr = QString("id:%1")
//		.arg(chr->equipmentId
//			.toString(QUuid::WithoutBraces)
//			.toLower());
//
//	const auto title = QString("title:%1")
//		.arg(chr->name);
//
//	const auto fullTarget = QString("target:%1")
//		.arg(target);
//
//	applicationController->executeCommandByName("window-create", QStringList{
//		fullTarget, itemIdStr, title });
//}

bool UserWindow::handleCommand(const std::shared_ptr<Instruction> cmd, CommandContext* context) {
	const auto action = cmd->parameters.value("action");
	if (!action.isNull() && action == "create") {
		auto services = context->services();
		d->usersService = services->get<UsersService>();
		d->imagesService = services->get<ImagesService>();

		if (!d->usersService) {
			context->printError("Unsupported Users service");
			return false;
		}

		if (!d->imagesService) {
			context->printError("Unsupported Images service");
			return false;
		}

		d->commandController = context->commandController();
		d->userWidget = new UserWidget(d->usersService, d->imagesService, this);

		// Подключаем сигналы
		connect(d->usersService, &UsersService::loggedOut, this, &UserWindow::onUserLoggedOut);

		setWidget(d->userWidget);

		return true;
	}

	return false;
}

void UserWindow::onUserLoggedOut() {
	close();
}
