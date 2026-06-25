#include "Content/CharactersModule/services/characters_service.h"
#include "Content/CharactersModule/data_providers/i_character_data_provider.h"
#include "Content/CharactersModule/models/character.h"
#include "Content/TexturesModule/services/images_service.h"
#include "Content/TexturesModule/data_providers/i_images_data_provider.h"

#include <map>

class CharactersService::Private {
public:
	Private(CharactersService* parent) : q(parent) {}
	CharactersService* q;

	ICharacterDataProvider* characterDataProvider;
	ImagesService* imagesService;
	std::map<QUuid, std::shared_ptr<Character>> characters;

	void loadCharacters();
};

CharactersService::CharactersService(
	ICharacterDataProvider* characterDataProvider,
	ImagesService* imagesService,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->characterDataProvider = characterDataProvider;
	d->imagesService = imagesService;
}

CharactersService::~CharactersService() = default;


std::shared_ptr<Character> CharactersService::getCharacter(const QUuid& characterId) const {
	auto it = d->characters.find(characterId);
	if (it != d->characters.end()) {
		return it->second;
	}

	return nullptr;
}

std::list<QUuid> CharactersService::getAllCharacterIds() const {
	std::list<QUuid> characterIds;
	for (const auto& [id, handler] : d->characters) {
		characterIds.push_back(id);
	}
	return characterIds;
}

/// Загрузить всех персонажей текущего пользователя
void CharactersService::Private::loadCharacters() {
	characters.clear();

	//auto user = characterDataProvider->loadUser(currentUserId);
	//if (!user) {
	//	return;
	//}

	//for (auto& characterId : user->characters) {
	//	auto character = characterDataProvider->loadCharacter(characterId);
	//	if (!character) {
	//		continue;
	//	}

	//	if (!character->iconPath.isEmpty()) {
	//		character->icon = imagesService->getImage(character->iconPath, ImageType::Character);
	//	}

	//	characters[characterId] = character;
	//}
}
