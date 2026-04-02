#include "ApplicationLayer/users/users_service.h"
#include "ApplicationLayer/character/character_item_handler.h"
#include "DataLayer/users/user.h"
#include <QCryptographicHash>
#include <QUuid>

class UsersService::Private {
public:
	Private(UsersService* parent)
		: q(parent) {
	}

	UsersService* q;
	IUsersDataProvider* usersDataProvider = nullptr;
	ICharacterDataProvider* characterDataProvider = nullptr;
	QString currentUserId;
	bool authenticated = false;
	std::map<QUuid, std::unique_ptr<CharacterItemHandler>> characters;

	/// Создать хэш пароля
	static QString hashPassword(const QString& password) {
		return QCryptographicHash::hash(password.toUtf8(), QCryptographicHash::Sha256).toHex();
	}

	static QString hashLogin(const QString& login) {
		return QCryptographicHash::hash(login.toUtf8(), QCryptographicHash::Sha256).toHex();
	}

	/// Загрузить всех персонажей текущего пользователя
	void loadCharacters() {
		characters.clear();
		
		auto userOpt = usersDataProvider->loadUser(currentUserId);
		if (!userOpt.has_value()) {
			return;
		}

		const auto& user = userOpt.value();
		for (const auto& characterId : user.characters) {
			Character characterData;
			if (characterDataProvider->loadCharacter(characterId, characterData)) {
				auto handler = std::make_unique<CharacterItemHandler>();
				handler->id = characterData.id;
				handler->name = characterData.name;
				handler->equipmentId = characterData.equipmentId;
				handler->inventoryId = characterData.inventoryId;
				characters.emplace(characterId, std::move(handler));
			}
		}
	}
};

UsersService::UsersService(IUsersDataProvider* usersDataProvider, ICharacterDataProvider* characterDataProvider, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->usersDataProvider = usersDataProvider;
	d->characterDataProvider = characterDataProvider;
}

UsersService::~UsersService() = default;

std::optional<QString> UsersService::login(const QString& login, const QString& password) {
	const auto loginHash = Private::hashLogin(login);
	auto userOpt = d->usersDataProvider->loadUser(loginHash);
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

	// Загружаем персонажей текущего пользователя
	d->loadCharacters();

	emit loginSuccess(user.displayName);
	return user.loginHash;
}

void UsersService::logout() {
	d->currentUserId.clear();
	d->authenticated = false;
	d->characters.clear();
	emit loggedOut();
}

bool UsersService::isAuthenticated() const {
	return d->authenticated;
}

std::optional<UserData> UsersService::currentUser() const {
	if (!d->authenticated || d->currentUserId.isEmpty()) {
		return std::nullopt;
	}

	return d->usersDataProvider->loadUser(d->currentUserId);
}

QString UsersService::currentUserId() const {
	return d->currentUserId;
}

CharacterItemHandler* UsersService::getCharacter(const QUuid& characterId) const {
	if (!d->authenticated) {
		return nullptr;
	}

	auto it = d->characters.find(characterId);
	if (it != d->characters.end()) {
		return it->second.get();
	}
	return nullptr;
}

std::optional<QString> UsersService::registerUser(const QString& login, const QString& password, const QString& displayName) {
	const auto loginHash = Private::hashLogin(login);
	const auto existingUser = d->usersDataProvider->loadUser(loginHash);
	if (existingUser.has_value()) {
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
