#pragma once
#include "Graphics/textures/texture_region.h"
#include "Graphics/textures/texture_filter.h"
#include "Graphics/textures/tile_data.h"
#include <QOpenGLFunctions_3_3_Core>
#include <memory>

class QOpenGLTexture;
class QPixmap;
class UploadedTexture;

/// Тексурный атлас - загружает тайлсет в одну большую OpenGL текстуру
/// и рассчитывает UV координаты для каждого тайла
class TextureAtlas {
public:
	/// Конструктор
	TextureAtlas();
	~TextureAtlas();

	bool load(std::shared_ptr<UploadedTexture> texture, int tilesCountX, int tilesCountY);

	/// Получить UV区域 для конкретного тайла
	/// @param tileX Позиция тайла по X (0..tilesCountX-1)
	/// @param tileY Позиция тайла по Y (0..tilesCountY-1)
	TextureRegion getRegion(int tileX, int tileY) const;

	/// Забиндить текстуру для рендеринга
	void bind() const;

	/// Отбиндить текстуру
	void unbind() const;

	/// Размеры
	int tileSizeX() const;
	int tileSizeY() const;
	int tilesCountX() const;
	int tilesCountY() const;

	bool isLoaded() const;

	const UploadedTexture* texture() const;

	TileData getTile(int tileX, int tileY) const;
	TileData getTileById(int tileId) const;
	int totalTiles() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
