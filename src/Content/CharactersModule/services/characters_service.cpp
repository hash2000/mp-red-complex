#include "Content/CharactersModule/services/characters_service.h"
#include "Content/CharactersModule/data_providers/i_character_data_provider.h"
#include "Content/CharactersModule/models/character.h"
#include "Content/UsersModule/models/user_view.h"
#include "Content/UsersModule/services/users_service.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Content/TexturesModule/data_providers/i_images_data_provider.h"

#include <map>

class CharactersService::Private {
public:
	Private(CharactersService* parent) : q(parent) {}
	CharactersService* q;

	ICharacterDataProvider* characterDataProvider;
	UsersService* usersService;
	ImagesService* imagesService;
	std::map<QUuid, std::shared_ptr<Character>> characters;

	void loadContent(std::shared_ptr<Character> chr);
	bool checkPermissions(const QString& userId);
};

CharactersService::CharactersService(
	ICharacterDataProvider* characterDataProvider,
	UsersService* usersService,
	ImagesService* imagesService,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->characterDataProvider = characterDataProvider;
	d->usersService = usersService;
	d->imagesService = imagesService;
}

CharactersService::~CharactersService() = default;

void CharactersService::Private::loadContent(std::shared_ptr<Character> chr) {
	if (!chr->iconPath.isEmpty()) {
		chr->icon = imagesService->getImage(chr->iconPath, ImageType::Character);
	}
}

bool CharactersService::Private::checkPermissions(const QString& userId) {

	const auto user = usersService->currentUser();

	if (userId != user->data->loginHash && !user->permissionsFlags.charactersView) {
		qWarning() << "Access denied";
		return false;
	}

	return true;
}

std::shared_ptr<Character> CharactersService::character(const QUuid& characterId) const {
	if (!d->usersService->isAuthenticated()) {
		qWarning() << "The user is not logged in";
		return std::shared_ptr<Character>();
	}

	auto chr = d->characterDataProvider->character(characterId);
	if (!chr) {
		qWarning() << "The user cannot be found" << characterId;
		return std::shared_ptr<Character>();
	}

	if (!d->checkPermissions(chr->userId)) {
		return std::shared_ptr<Character>();
	}

	d->loadContent(chr);

	return chr;
}

std::list<std::shared_ptr<Character>> CharactersService::charactersByUser(const QString& userId) const {
	if (!d->usersService->isAuthenticated()) {
		qWarning() << "The user is not logged in";
		return std::list<std::shared_ptr<Character>>();
	}

	if (!d->checkPermissions(userId)) {
		return std::list<std::shared_ptr<Character>>();
	}

	auto chrs = d->characterDataProvider->userCharacters(userId);
	for (auto chr : chrs) {
		d->loadContent(chr);
	}

	return chrs;
}
