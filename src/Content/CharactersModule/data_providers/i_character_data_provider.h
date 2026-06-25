#pragma once
#include <QUuid>
#include <optional>

class Character;

class ICharacterDataProvider {
public:
	virtual ~ICharacterDataProvider() = default;

	virtual std::shared_ptr<Character> userCharacter(const QUuid& id) const = 0;
	virtual std::shared_ptr<Character> character(const QUuid& id) const = 0;
	virtual bool saveCharacter(const Character& character) = 0;
	virtual bool deleteCharacter(const QUuid& id) = 0;
};
