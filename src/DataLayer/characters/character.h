#pragma once
#include <QUuid>
#include <QString>
#include <array>

enum class CharacterStats {
	Intellect,
	Strength,
	Endurance,
	Agility,

	Count,
};

class Character {
public:
	QUuid id;
	QString name;
	QUuid equipmentId;
	QUuid inventoryId;
};
