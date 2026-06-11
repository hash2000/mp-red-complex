#pragma once
#include "Content/UsersModule/data_providers/characters/i_character_data_provider.h"
#include <memory>

class DatabasesService;

class CharacterDataProviderDb : public ICharacterDataProvider {
public:
	CharacterDataProviderDb(DatabasesService* databasesService);
	~CharacterDataProviderDb() override;

	std::optional<Character> loadCharacter(const QUuid& id) const override;
	bool saveCharacter(const Character& character) override;
	bool deleteCharacter(const QUuid& id) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
