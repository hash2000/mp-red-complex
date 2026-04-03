#pragma once
#include <QUuid>
#include <QString>
#include <QPixmap>
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

  QString iconPath;
  QPixmap icon;
  int level = 1;
};
