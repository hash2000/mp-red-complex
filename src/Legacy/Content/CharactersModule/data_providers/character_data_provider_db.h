#pragma once
#include "Content/CharactersModule/data_providers/i_character_data_provider.h"
#include <memory>

class DatabasesService;

class CharacterDataProviderDb : public ICharacterDataProvider {
public:
	CharacterDataProviderDb(DatabasesService* databasesService);
	~CharacterDataProviderDb() override;

	std::list<std::shared_ptr<Character>> userCharacters(const QString& userId) const override;
	std::shared_ptr<Character> character(const QUuid& id) const override;
	bool saveCharacters(const QString& userId, const std::list<std::shared_ptr<Character>>& characters) override;
	bool deleteCharacter(const QUuid& id) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
