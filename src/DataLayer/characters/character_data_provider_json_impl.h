#pragma once
#include "DataLayer/characters/i_character_data_provider.h"
#include <memory>

class Resources;

class CharacterDataProviderJsonImpl : public ICharacterDataProvider {
public:
	CharacterDataProviderJsonImpl(Resources* resources);
	~CharacterDataProviderJsonImpl() override;

	bool loadCharacter(const QUuid& id, Character& character) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
