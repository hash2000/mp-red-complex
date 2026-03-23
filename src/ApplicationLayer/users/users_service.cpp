#include "ApplicationLayer/users/users_service.h"
#include <QCryptographicHash>
#include <QUuid>

class UsersService::Private {
public:
	Private(UsersService* parent)
		: q(parent) {
	}

	UsersService* q;
	IUsersDataProvider* dataProvider = nullptr;
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

UsersService::UsersService(IUsersDataProvider* dataProvider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
}

UsersService::~UsersService() = default;

std::optional<QString> UsersService::login(const QString& login, const QString& password) {
	const auto loginHash = Private::hashLogin(login);
	auto userOpt = d->dataProvider->loadUser(loginHash);
	if (!userOpt.has_value()) {
		return std::nullopt;
	}

	const auto& user = userOpt.value();
	QString passwordHash = Private::hashPassword(password);

	if (user.passwordHash != passwordHash) {
		return std::nullopt;
	}

	// Успешный вход
	d->currentUserId = user.loginHash;
	d->authenticated = true;

	emit loginSuccess(user.displayName);
	return user.loginHash;
}

void UsersService::logout() {
	d->currentUserId.clear();
	d->authenticated = false;
	emit loggedOut();
}

bool UsersService::isAuthenticated() const {
	return d->authenticated;
}

std::optional<UserData> UsersService::currentUser() const {
	if (!d->authenticated || d->currentUserId.isEmpty()) {
		return std::nullopt;
	}

	return d->dataProvider->loadUser(d->currentUserId);
}

QString UsersService::currentUserId() const {
	return d->currentUserId;
}

std::optional<QString> UsersService::registerUser(const QString& login, const QString& password, const QString& displayName) {
	const auto loginHash = Private::hashLogin(login);
	const auto existingUser = d->dataProvider->loadUser(loginHash);
	if (existingUser.has_value()) {
		return std::nullopt;
	}

	UserData user;
	user.loginHash = loginHash;
	user.passwordHash = Private::hashPassword(password);
	user.displayName = displayName.isEmpty() ? login : displayName;

	if (!d->dataProvider->saveUser(user)) {
		return std::nullopt;
	}

	return user.loginHash;
}

QString UsersService::hashPassword(const QString& password) {
	return Private::hashPassword(password);
}
