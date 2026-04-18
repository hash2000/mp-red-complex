#pragma once
#include "Graphics/tiles/tile_data.h"
#include <memory>

class TextureAtlas;

/// Набор тайлов - управляет связью между ID тайла и UV region в атласе
class Tileset {
public:
	Tileset();
	~Tileset();

	/// Инициализировать тайлсет с атласом
	/// @param atlas Указатель на TextureAtlas (не владеет)
	/// @param tilesCountX Количество тайлов по X
	/// @param tilesCountY Количество тайлов по Y
	void initialize(TextureAtlas* atlas, int tilesCountX, int tilesCountY);

	/// Получить TileData по позиции в тайлсете
	TileData getTile(int tileX, int tileY) const;

	/// Получить TileData по ID
	TileData getTileById(int tileId) const;

	/// Проверка инициализации
	bool isInitialized() const;

	/// Атлас текстур
	TextureAtlas* atlas() const;

	/// Размеры
	int tilesCountX() const;
	int tilesCountY() const;
	int totalTiles() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
