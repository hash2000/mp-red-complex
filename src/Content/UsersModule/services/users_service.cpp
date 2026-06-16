#include "Content/UsersModule/services/users_service.h"
#include "Content/UsersModule/data_providers/users/i_users_data_provider.h"
#include "Content/UsersModule/data_providers/characters/i_character_data_provider.h"
#include "Content/UsersModule/models/character.h"
#include "Content/UsersModule/models/user.h"
#include "ApplicationLayer/textures/images_service.h"

#include <QCryptographicHash>
#include <QUuid>

class UsersService::Private {
public:
	Private(UsersService* parent) : q(parent) { }
	UsersService* q;

	IUsersDataProvider* usersDataProvider = nullptr;
	ICharacterDataProvider* characterDataProvider = nullptr;
	ImagesService* imagesService = nullptr;
	QString currentUserId;
	QUuid chestId;
	bool authenticated = false;
	std::map<QUuid, std::shared_ptr<Character>> characters;

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
		
		auto user = usersDataProvider->loadUser(currentUserId);
		if (!user) {
			return;
		}

		for (auto& characterId : user->characters) {
			auto character = characterDataProvider->loadCharacter(characterId);
			if (!character) {
				continue;
			}

			if (!character->iconPath.isEmpty()) {
				character->icon = imagesService->getImage(character->iconPath, ImageType::Character);
			}

			characters[characterId] = character;			
		}
	}
};

UsersService::UsersService(
	IUsersDataProvider* usersDataProvider,
	ICharacterDataProvider* characterDataProvider,
	ImagesService* ImagesService,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->usersDataProvider = usersDataProvider;
	d->characterDataProvider = characterDataProvider;
	d->imagesService = ImagesService;
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
	d->chestId = user->chestId;
	d->authenticated = true;

	// Загружаем персонажей текущего пользователя
	d->loadCharacters();

	emit loginSuccess(*user);
	return user->loginHash;
}

void UsersService::logout() {
	d->currentUserId.clear();
	d->authenticated = false;
	d->characters.clear();
	d->chestId = QUuid();
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

std::shared_ptr<Character> UsersService::getCharacter(const QUuid& characterId) const {
	if (!d->authenticated) {
		return nullptr;
	}

	auto it = d->characters.find(characterId);
	if (it != d->characters.end()) {
		return it->second;
	}

	return nullptr;
}

std::list<QUuid> UsersService::getAllCharacterIds() const {
	std::list<QUuid> characterIds;
	if (!d->authenticated) {
		return characterIds;
	}

	for (const auto& [id, handler] : d->characters) {
		characterIds.push_back(id);
	}
	return characterIds;
}

QUuid UsersService::getChestId() const {
	return d->chestId;
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
