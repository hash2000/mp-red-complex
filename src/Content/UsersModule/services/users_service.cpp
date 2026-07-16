#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/data_providers/users/i_users_data_provider.h"
#include "Content/UsersModule/models/user_view.h"
#include "Content/UsersModule/context/current_user_context.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Content/TexturesModule/data_providers/i_images_data_provider.h"

#include <QCryptographicHash>
#include <QUuid>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
//#include <sodium.h>

class UsersService::Private {
public:
	Private(UsersService* parent) : q(parent) { }
	UsersService* q;

	std::unique_ptr<CurrentUserContext> currentUserContext;
	IUsersDataProvider* usersDataProvider = nullptr;
	ImagesService* imagesService = nullptr;
	Resources* resources;

	/// Создать хэш пароля
	static QString hashPassword(const QString& password) {
		return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
	}

	static QString hashLogin(const QString& login) {
		return QCryptographicHash::hash(login.toUtf8(), QCryptographicHash::Sha256).toHex();
	}

	std::shared_ptr<UserView> loadUser(const QString& id);
};

UsersService::UsersService(
	Resources* resources,
	IUsersDataProvider* usersDataProvider,
	ImagesService* imagesService,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->usersDataProvider = usersDataProvider;
	d->imagesService = imagesService;
	d->currentUserContext = std::make_unique<CurrentUserContext>(resources);
}

UsersService::~UsersService() = default;

std::optional<QString> UsersService::login(const QString& login, const QString& password) {
	if (isAuthenticated()) {
		logout();
	}

	const auto loginHash = Private::hashLogin(login);

	//d->currentUserContext->setCurrentUser(loginHash, );

	//auto user = d->loadUser(loginHash);
	//if (!user) {
	//	qWarning() << "User not found" << login;
	//	return std::nullopt;
	//}

	//QString passwordHash = Private::hashPassword(password);

	//if (user->data->passwordHash != passwordHash) {
	//	return std::nullopt;
	//}

	//// Успешный вход
	//d->currentUser = user;

	//emit loginSuccess(*user);

	//return user->data->loginHash;

//	d->resources->Variables.set("Users.CurrentUser.Identity.Name", "guest");
	return std::nullopt;
}

void UsersService::logout() {
	//d->currentUser.reset();
	emit loggedOut();
}

bool UsersService::isAuthenticated() const {
	return false;// (bool)d->currentUser;
}

std::shared_ptr<UserView> UsersService::Private::loadUser(const QString& id) {
	if (id.isEmpty()) {
		return std::shared_ptr<UserView>();
	}

	auto userData = usersDataProvider->loadUser(id);
	if (!userData) {
		return std::shared_ptr<UserView>();
	}

	if (!userData->iconPath.isEmpty()) {
		userData->icon = imagesService->getImage(userData->iconPath, ImageType::Users);
	}

	auto view = std::make_shared<UserView>();
	view->data = userData;
	view->permissions = usersDataProvider->permissions(id);
	view->permissionsFlags.usersCreate = view->permissions.find("--users-create") != view->permissions.end();
	view->permissionsFlags.usersView  = view->permissions.find("--users-view") != view->permissions.end();
	view->permissionsFlags.userCanRegisterInGame = view->permissions.find("--user-can-register-in-game") != view->permissions.end();
	view->permissionsFlags.userCanRegisterInMessages = view->permissions.find("--user-can-register-in-messages") != view->permissions.end();
	view->permissionsFlags.charactersView = view->permissions.find("--characters-view") != view->permissions.end();

	return view;
}

CurrentUserContext* UsersService::currentUserContext() {
	return d->currentUserContext.get();
}
