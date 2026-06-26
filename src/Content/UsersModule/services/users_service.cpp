#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/data_providers/users/i_users_data_provider.h"
#include "Content/UsersModule/models/user_view.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Content/TexturesModule/data_providers/i_images_data_provider.h"

#include <QCryptographicHash>
#include <QUuid>

class UsersService::Private {
public:
	Private(UsersService* parent) : q(parent) { }
	UsersService* q;

	IUsersDataProvider* usersDataProvider = nullptr;
	ImagesService* imagesService = nullptr;
	std::shared_ptr<UserView> currentUser;

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
	IUsersDataProvider* usersDataProvider,
	ImagesService* imagesService,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->usersDataProvider = usersDataProvider;
	d->imagesService = imagesService;
}

UsersService::~UsersService() = default;

std::optional<QString> UsersService::login(const QString& login, const QString& password) {
	if (isAuthenticated()) {
		logout();
	}

	const auto loginHash = Private::hashLogin(login);
	auto user = d->loadUser(loginHash);
	if (!user) {
		qWarning() << "User not found" << login;
		return std::nullopt;
	}

	QString passwordHash = Private::hashPassword(password);

	if (user->data->passwordHash != passwordHash) {
		return std::nullopt;
	}

	// Успешный вход
	d->currentUser = user;

	emit loginSuccess(*user);

	return user->data->loginHash;
}

void UsersService::logout() {
	d->currentUser.reset();
	emit loggedOut();
}

bool UsersService::isAuthenticated() const {
	return (bool)d->currentUser;
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

std::shared_ptr<UserView> UsersService::currentUser() const {
	return d->currentUser;
}

QString UsersService::currentUserId() const {
	if (!d->currentUser) {
		return QString();
	}

	return d->currentUser->data->loginHash;
}

std::optional<QString> UsersService::registerUser(const QString& login, const QString& password, const QString& displayName) {
	const auto loginHash = Private::hashLogin(login);
	auto containsUser = d->usersDataProvider->containsUser(loginHash);
	if (containsUser) {
		return std::nullopt;
	}

	auto user = std::make_shared<UserData>();
	user->loginHash = loginHash;
	user->passwordHash = Private::hashPassword(password);
	user->displayName = displayName.isEmpty() ? login : displayName;

	if (!d->usersDataProvider->saveUser(user)) {
		return std::nullopt;
	}

	return user->loginHash;
}
