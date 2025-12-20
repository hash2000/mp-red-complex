#pragma once
#include <QImage>
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

struct FrameData {
  QImage image; // ARGB32, с прозрачностью (0 = transparent)
  int offsetX = 0;
  int offsetY = 0;
  int width = 0;
  int height = 0;
};

struct AtlasEntry {
  int x = 0, y = 0;
  int width = 0;
	int height = 0;
  float u0 = 0, v0 = 0, u1 = 1, v1 = 1;
  int offsetX = 0;
	int offsetY = 0;
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
