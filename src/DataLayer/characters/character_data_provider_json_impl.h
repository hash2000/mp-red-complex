#pragma once
#include "DataLayer/characters/i_character_data_provider.h"
#include <memory>

class Resources;

class CharacterDataProviderJsonImpl : public ICharacterDataProvider {
public:
	CharacterDataProviderJsonImpl(Resources* resources);
	~CharacterDataProviderJsonImpl() override;

	bool loadCharacter(const QUuid& id, Character& character) const override;
	bool saveCharacter(const Character& character) override;
	bool deleteCharacter(const QUuid& id) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
