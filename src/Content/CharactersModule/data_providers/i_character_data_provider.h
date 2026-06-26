#pragma once
#include <QUuid>
#include <optional>
#include <memory>
#include <list>

class Character;

class ICharacterDataProvider {
public:
	virtual ~ICharacterDataProvider() = default;

	virtual std::list<std::shared_ptr<Character>> userCharacters(const QString& userId) const = 0;
	virtual std::shared_ptr<Character> character(const QUuid& id) const = 0;
	virtual bool saveCharacters(const QString& userId, const std::list<std::shared_ptr<Character>>& characters) = 0;
	virtual bool deleteCharacter(const QUuid& id) = 0;
};
