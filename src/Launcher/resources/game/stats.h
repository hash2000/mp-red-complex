#pragma once
#include "Base/format.h"
#include "Base/from.h"
#include <cstdint>

enum class StatId : uint32_t {
  Strength = 0,
  Perception,
  Endurance,
  Charisma,
  Intelligence,
  Agility,
  Luck,
};

template <> class Format<StatId> {
  static QString format(const StatId &value) {
    switch (value) {
    case StatId::Strength:
      return QStringLiteral("Strength");
    case StatId::Perception:
      return QStringLiteral("Perception");
    case StatId::Endurance:
      return QStringLiteral("Endurance");
    case StatId::Charisma:
      return QStringLiteral("Charisma");
    case StatId::Intelligence:
      return QStringLiteral("Intelligence");
    case StatId::Agility:
      return QStringLiteral("Agility");
    case StatId::Luck:
      return QStringLiteral("Luck");
    }
    return QStringLiteral("Unknown stat");
  }
};

template <> struct From<StatId> {
  static std::optional<StatId> from(const QString &value) {
    if (value == QStringLiteral("Strength")) {
      return StatId::Strength;
    } else if (value == QStringLiteral("Perception")) {
      return StatId::Perception;
    } else if (value == QStringLiteral("Endurance")) {
      return StatId::Endurance;
    } else if (value == QStringLiteral("Charisma")) {
      return StatId::Charisma;
    } else if (value == QStringLiteral("Intelligence")) {
      return StatId::Intelligence;
    } else if (value == QStringLiteral("Agility")) {
      return StatId::Agility;
    } else if (value == QStringLiteral("Strength")) {
      return StatId::Strength;
    } else if (value == QStringLiteral("Luck")) {
      return StatId::Luck;
    }
    return std::nullopt;
  }
};
