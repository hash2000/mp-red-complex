#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/data_providers/users/i_users_data_provider.h"
#include "Content/UsersModule/models/user.h"
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
	QString currentUserId;
	bool authenticated = false;

	/// Создать хэш пароля
	static QString hashPassword(const QString& password) {
		return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
	}

	static QString hashLogin(const QString& login) {
		return QCryptographicHash::hash(login.toUtf8(), QCryptographicHash::Sha256).toHex();
	}
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
	const auto loginHash = Private::hashLogin(login);
	auto user = d->usersDataProvider->loadUser(loginHash);
	if (!user) {
		return std::nullopt;
	}

	QString passwordHash = Private::hashPassword(password);

	if (user->passwordHash != passwordHash) {
		return std::nullopt;
	}

	// Успешный вход
	d->currentUserId = user->loginHash;
	d->authenticated = true;

	emit loginSuccess(*user);
	return user->loginHash;
}

void UsersService::logout() {
	d->currentUserId.clear();
	d->authenticated = false;
	emit loggedOut();
}

bool UsersService::isAuthenticated() const {
	return d->authenticated;
}

std::shared_ptr<UserData> UsersService::currentUser() const {
	if (!d->authenticated || d->currentUserId.isEmpty()) {
		return std::shared_ptr<UserData>();
	}

	auto userData = d->usersDataProvider->loadUser(d->currentUserId);
	if (userData) {
		if (!userData->iconPath.isEmpty()) {
			userData->icon = d->imagesService->getImage(userData->iconPath, ImageType::Users);
		}
	}

	return userData;
}

QString UsersService::currentUserId() const {
	return d->currentUserId;
}

std::optional<QString> UsersService::registerUser(const QString& login, const QString& password, const QString& displayName) {
	const auto loginHash = Private::hashLogin(login);
	auto containsUser = d->usersDataProvider->containsUser(loginHash);
	if (containsUser) {
		return std::nullopt;
	}

	UserData user;
	user.loginHash = loginHash;
	user.passwordHash = Private::hashPassword(password);
	user.displayName = displayName.isEmpty() ? login : displayName;

	if (!d->usersDataProvider->saveUser(user)) {
		return std::nullopt;
	}

	return user.loginHash;
}

QString UsersService::hashPassword(const QString& password) {
	return Private::hashPassword(password);
}
