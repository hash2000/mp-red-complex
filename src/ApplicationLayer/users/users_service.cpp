#include "ApplicationLayer/users/users_service.h"
#include <QCryptographicHash>
#include <QUuid>

class UsersService::Private {
public:
	Private(UsersService* parent)
		: q(parent) {
	}

	UsersService* q;
	std::unique_ptr<IUsersDataProvider> dataProvider;
	QString currentUserId;
	bool authenticated = false;

	/// Создать хэш пароля
	static QString hashPassword(const QString& password) {
		return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
	}
};

UsersService::UsersService(std::unique_ptr<IUsersDataProvider> dataProvider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->dataProvider = std::move(dataProvider);
}

UsersService::~UsersService() = default;

std::optional<QString> UsersService::login(const QString& login, const QString& password) {
	auto userOpt = d->dataProvider->findUserByLogin(login);
	if (!userOpt.has_value()) {
		return std::nullopt;
	}

	const auto& user = userOpt.value();
	QString passwordHash = Private::hashPassword(password);

	if (user.passwordHash != passwordHash) {
		return std::nullopt;
	}

	// Успешный вход
	d->currentUserId = user.id;
	d->authenticated = true;

	emit loginSuccess(user.id);
	return user.id;
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
	// Проверяем, существует ли пользователь с таким логином
	auto existingUser = d->dataProvider->findUserByLogin(login);
	if (existingUser.has_value()) {
		return std::nullopt; // Пользователь уже существует
	}

	// Создаём нового пользователя
	UserData user;
	user.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	user.login = login;
	user.passwordHash = Private::hashPassword(password);
	user.displayName = displayName.isEmpty() ? login : displayName;

	if (!d->dataProvider->saveUser(user)) {
		return std::nullopt;
	}

	return user.id;
}

QString UsersService::hashPassword(const QString& password) {
	return Private::hashPassword(password);
}
