#pragma once
#include <QUuid>

class Character;

class ICharacterDataProvider {
public:
	virtual ~ICharacterDataProvider() = default;

	virtual bool loadCharacter(const QUuid& id, Character& character) const = 0;
	virtual bool saveCharacter(const Character& character) = 0;
	virtual bool deleteCharacter(const QUuid& id) = 0;
};
