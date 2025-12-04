#pragma once
#include <QString>
#include <cstdint>
#include <vector>

namespace Proto {

enum class Stat : uint32_t {
  Strength,
  Perception,
  Endurance,
  Charisma,
  Intelligence,
  Agility,
  Luck,
};

enum class StatSecondary : uint32_t {
	HitPoints,
	ActionPoints,
	ArmorClass,
	UnarmedClass, // unused
	MeleDamage,
	CarryWeight,
	Sequence,
	HealingRate,
	CriticalChance,
	CriticalHitModifier,
};

enum class Damage : uint32_t {
  Normal,
  Laser,
  Fire,
  Plasma,
  Electrical,
  Emp,
  Explosive,
};

enum class DamageResistance : uint32_t {
  Normal,
  Laser,
  Fire,
  Plasma,
  Electrical,
  Emp,
  Explosive,
  Radiation,
  Poison,
};

enum class Skill : int32_t {
  None = -1,
  SmallGuns = 0,
  BigGuns,
  EnergyWeapons,
  Unarmed,
  MeleWeapons,
  Throwing,
  FirstAid,
  Doctor,
  Sneak,
  Lockpick,
  Steal,
  Traps,
  Science,
  Repair,
  Speech,
  Barter,
  Gambling,
  Outdoorsman,
};

enum class Trait : int32_t {
  None = -1,
  FastMetabolism = 0,
  Bruiser,
  SmallFrame,
  OneHanded,
  Finesse,
  Kamikaze,
  HeavyHanded,
  FastShot,
  Bloody_mess,
  Jinxed,
  GoodNatured,
  ChemReliant,
  ChemResistant,
  SexAppeal,
  Skilled,
  Gifted,
};

enum class Age {
	Current
};

enum class Gender : char {
  Male,
  Female,
};

enum class Tagged : uint32_t {
	FirstSkill,
	SecondSkill,
	ThirdSkill,
	FourthSkill,
	FirstTrait,
	SecondTrait,
	ThirdTrait,
};

struct Character {
	std::vector<uint32_t> stat;
	std::vector<uint32_t> statsBonus;
	std::vector<uint32_t> statsSecondary;
	std::vector<uint32_t> statsSecondaryBonus;
	std::vector<uint32_t> damage;
	std::vector<uint32_t> damageBonus;
	std::vector<uint32_t> damageResistance;
	std::vector<uint32_t> damageResistanceBonus;
	std::vector<uint32_t> skill;
	std::vector<uint32_t> tagged;
	QString name;
	uint32_t age;
	uint32_t ageBonus;
	Gender genderBonus;
	Gender gender;
};


template<class T> [[nodiscard]] uint32_t to_u32(const T& state) {
	return static_cast<uint32_t>(state);
}

template<class T> [[nodiscard]] T to_state(uint32_t state) {
	return static_cast<T>(state);
}

template <class T> struct Validator {
	static bool validate(const T& state, uint32_t value, const std::vector<uint32_t>& stats) = delete;
};

template <> struct Validator<Stat> {
	static bool validate(const Stat& state, uint32_t value, const std::vector<uint32_t>& stats) {
		return value <= 10;
	}
};

template <> struct Validator<StatSecondary> {
	static bool validate(const StatSecondary& state, uint32_t value, const std::vector<uint32_t>& stats) {
		switch (state) {
		case StatSecondary::HitPoints:
			return value <= 999;
		case StatSecondary::ActionPoints:
			return value <= 999;
		case StatSecondary::ArmorClass:
			return value <= 999;
		case StatSecondary::UnarmedClass:
		// unused
			return true;
		case StatSecondary::MeleDamage:
			return value <= 999;
		case StatSecondary::CarryWeight:
			return value <= 999;
		case StatSecondary::Sequence:
			return value <= 99;
		case StatSecondary::HealingRate:
			return value <= 99;
		case StatSecondary::CriticalChance:
			return value <= 100;
		case StatSecondary::CriticalHitModifier:
			return value <= 100;
		}

		return false;
	}
};

template <> struct Validator<Damage> {
	static bool validate(const Damage& state, uint32_t value, const std::vector<uint32_t>& stats) {
		switch (state) {
  	case Damage::Normal:
			return value <= 999;
  	case Damage::Laser:
			return value <= 999;
  	case Damage::Fire:
			return value <= 999;
  	case Damage::Plasma:
			return value <= 999;
  	case Damage::Electrical:
			return value <= 999;
  	case Damage::Emp:
			return value <= 999;
  	case Damage::Explosive:
			return value <= 999;
		}

		return false;
	}
};

template <> struct Validator<DamageResistance> {
	static bool validate(const DamageResistance& state, uint32_t value, const std::vector<uint32_t>& stats) {
		switch (state) {
  	case DamageResistance::Normal:
			return value <= 100;
  	case DamageResistance::Laser:
			return value <= 100;
  	case DamageResistance::Fire:
			return value <= 100;
  	case DamageResistance::Plasma:
			return value <= 100;
  	case DamageResistance::Electrical:
			return value <= 100;
  	case DamageResistance::Emp:
			return value <= 100;
  	case DamageResistance::Explosive:
			return value <= 100;
  	case DamageResistance::Radiation:
			return value <= 100;
  	case DamageResistance::Poison:
			return value <= 100;
		}

		return false;
	}
};

float calculate_skill(const Skill& skill, uint32_t value, const std::vector<uint32_t>& stats);

template <> struct Validator<Skill> {
	static bool validate(const Skill& skill, uint32_t value, const std::vector<uint32_t>& stats) {
		return value <= calculate_skill(skill, 300, stats);
	}
};

template <> struct Validator<Age> {
	static bool validate(const Age& state, uint32_t value, const std::vector<uint32_t>& stats) {
		switch (state) {
  	case Age::Current:
			return value <= 35;
		}

		return false;
	}
};

template <> struct Validator<Tagged> {
	static bool validate(const Tagged& state, uint32_t value, const std::vector<uint32_t>& stats) {
		switch (state) {
		case Tagged::FirstSkill:
			return value <= 17;
		case Tagged::SecondSkill:
			return value <= 17;
		case Tagged::ThirdSkill:
			return value <= 17;
		case Tagged::FourthSkill:
			return value <= 17 || value == 0xffffffff;
		case Tagged::FirstTrait:
			return value <= 15;
		case Tagged::SecondTrait:
			return value <= 15;
		case Tagged::ThirdTrait:
			return value <= 70;
		}

		return false;
	}
};

} // namespace Proto
