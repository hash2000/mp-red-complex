#include "DataFormat/proto/gcd.h"

namespace Proto {
float calculate_skill(const Skill& skill, uint32_t value, const std::vector<uint32_t>& stats) {
	switch (skill) {
	case Skill::None:
			break;
	case Skill::SmallGuns:
			return value - (5 + 4 * stats[to_u32(Stat::Agility)]);
	case Skill::BigGuns:
			return value - 2 * stats[to_u32(Stat::Agility)];
	case Skill::EnergyWeapons:
			return value - 2 * stats[to_u32(Stat::Agility)];
	case Skill::Unarmed:
			return value - (30 + 2 * (stats[to_u32(Stat::Strength)] + stats[to_u32(Stat::Agility)]));
	case Skill::MeleWeapons:
			return value - (20 + 2 * (stats[to_u32(Stat::Strength)] + stats[to_u32(Stat::Agility)]));
	case Skill::Throwing:
			return value - 4 * stats[to_u32(Stat::Agility)];
	case Skill::FirstAid:
			return value - 2 * (stats[to_u32(Stat::Perception)] + stats[to_u32(Stat::Intelligence)]);
	case Skill::Doctor:
			return value - 5 + stats[to_u32(Stat::Perception)] + stats[to_u32(Stat::Intelligence)];
	case Skill::Sneak:
			return value - 5 + 3 * stats[to_u32(Stat::Agility)];
	case Skill::Lockpick:
			return value - 10 + stats[to_u32(Stat::Perception)] + stats[to_u32(Stat::Intelligence)];
	case Skill::Steal:
			return value - 3 * stats[to_u32(Stat::Agility)];
	case Skill::Traps:
			return value - 10 + stats[to_u32(Stat::Perception)] + stats[to_u32(Stat::Intelligence)];
	case Skill::Science:
			return value - 4 * stats[to_u32(Stat::Intelligence)];
	case Skill::Repair:
			return value - 3 * stats[to_u32(Stat::Intelligence)];
	case Skill::Speech:
			return value - 5 * stats[to_u32(Stat::Charisma)];
	case Skill::Barter:
			return value - 4 * stats[to_u32(Stat::Charisma)];
	case Skill::Gambling:
			return value - 4 * stats[to_u32(Stat::Luck)];
	case Skill::Outdoorsman:
			return value - 2 * (stats[to_u32(Stat::Endurance)] + stats[to_u32(Stat::Intelligence)]);
	}
	return 0.f;
}

}
