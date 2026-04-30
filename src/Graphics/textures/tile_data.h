#pragma once
#include "Graphics/textures/texture_region.h"

/// Данные одного тайла
struct TileData {
	/// ID тайла (индекс в атласе: tileId = tileY * tilesCountX + tileX)
	int tileId = -1;

	/// UV координаты в атласе
	TextureRegion region;

	/// Пустой тайл
	static TileData empty() { return TileData(); }

	/// Проверка на пустоту
	bool isEmpty() const { return tileId < 0; }
};
