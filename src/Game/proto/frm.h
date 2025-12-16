#pragma once
#include <cstdint>
#include <vector>

namespace Proto {

enum class FrmType : char {
  Item = 0,
  Critter,
  Scenery,
  Wall,
  Tile,
  Misc,
  Interface,
  Inventory,
};

struct Frame {
	uint16_t width = 0;
	uint16_t height = 0;
	uint32_t frameSize = 0;
	int16_t offsetX = 0;
	int16_t offsetY = 0;
	std::vector<std::byte> indexes;
};

struct Direction {
	int16_t shiftX = 0;
	int16_t shiftY = 0;
	uint32_t dataOffset = 0;
	std::vector<Frame> frames;
};

struct Animation {
	uint32_t version;
	uint16_t fps; // frames per second
	uint16_t current;
	uint16_t fpd; // frames per direction
	uint16_t xShift;
	std::vector<Direction> directions;
};

} // namespace Proto
