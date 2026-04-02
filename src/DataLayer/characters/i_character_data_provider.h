#pragma once
#include <QUuid>

class Character;

class ICharacterDataProvider {
public:
	virtual ~ICharacterDataProvider() = default;

	virtual bool loadCharacter(const QUuid& id, Character& character) = 0;
};
