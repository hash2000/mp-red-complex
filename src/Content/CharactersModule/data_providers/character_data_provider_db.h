#pragma once
#include "Content/CharactersModule/data_providers/i_character_data_provider.h"
#include <memory>

class DatabasesService;

class CharacterDataProviderDb : public ICharacterDataProvider {
public:
	CharacterDataProviderDb(DatabasesService* databasesService);
	~CharacterDataProviderDb() override;

	std::shared_ptr<Character> userCharacter(const QUuid& id) const override;
	std::shared_ptr<Character> character(const QUuid& id) const override;
	bool saveCharacter(const Character& character) override;
	bool deleteCharacter(const QUuid& id) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
