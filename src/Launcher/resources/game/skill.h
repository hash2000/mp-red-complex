#pragma once
#include <cstdint>

enum class SkillId : int32_t {
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
