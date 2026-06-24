#pragma once
#include <cstdint>

namespace ShaderFeature {
	constexpr uint64_t None = 0x0;
	constexpr uint64_t UseTexCoord = 0x1;
	constexpr uint64_t GridOverlay = 0x2;
	constexpr uint64_t TileAnimation = 0x4;
	constexpr uint64_t AnimateUV = 0x8;
}

namespace ShaderFlags {
	constexpr uint64_t None = 0x0;
	constexpr uint64_t SolidColor = 0x1;      // Сплошной цвет (без текстуры)
	constexpr uint64_t AlphaTest = 0x2;				// Альфа-отбрасывание
	constexpr uint64_t Specular = 0x4;				// Блик
	constexpr uint64_t Wet = 0x8;							// Мокрый пол
	constexpr uint64_t Fire = 0x10;						// Огонь
}
