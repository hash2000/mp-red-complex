#pragma once
#include "Game/data_format/gcd/gcd.h"

namespace DataFormat::Gcd {
template <class Enum>
struct EnumTraits {
    static constexpr Enum first = 0;
    static constexpr Enum last  = 0;
};

template <> struct EnumTraits<Stat> {
  static constexpr Stat first = Stat::Strength;
  static constexpr Stat last = Stat::Luck;
};

template <> struct EnumTraits<StatSecondary> {
  static constexpr StatSecondary first = StatSecondary::HitPoints;
  static constexpr StatSecondary last = StatSecondary::CriticalHitModifier;
};

template <> struct EnumTraits<Damage> {
  static constexpr Damage first = Damage::Normal;
  static constexpr Damage last = Damage::Explosive;
};

template <> struct EnumTraits<DamageResistance> {
  static constexpr DamageResistance first = DamageResistance::Normal;
  static constexpr DamageResistance last = DamageResistance::Poison;
};

template <> struct EnumTraits<Skill> {
  static constexpr Skill first = Skill::SmallGuns;
  static constexpr Skill last = Skill::Outdoorsman;
};

template <> struct EnumTraits<Trait> {
  static constexpr Trait first = Trait::FastMetabolism;
  static constexpr Trait last = Trait::Gifted;
};

template <> struct EnumTraits<Gender> {
  static constexpr Gender first = Gender::Male;
  static constexpr Gender last = Gender::Female;
};

template <> struct EnumTraits<Tagged> {
  static constexpr Tagged first = Tagged::FirstSkill;
  static constexpr Tagged last = Tagged::ThirdTrait;
};

}
