#pragma once
#include <cstdint>

enum class DamageId : uint32_t {
    Normal = 0,
    Laser,
    Fire,
    Plasma,
    Electrical,
    Emp,
    Explosive,
    Radiation,
    Poison,
};
