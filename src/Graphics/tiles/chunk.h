#pragma once
#include "Graphics/textures/tile_data.h"
#include <QPoint>
#include <QSize>
#include <QColor>
#include <memory>

class TextureAtlas;

/// Чанк - блок тайлов NxN с собственным VAO/VBO
/// Координаты: XZ плоскость, Y = высота
class Chunk {
public:
	/// Размер чанка по умолчанию
	static constexpr int kDefaultChunkSize = 32;

	Chunk(int layers);
	~Chunk();

	/// Установить позицию чанка в мире (в чанках, не в пикселях)
	void setChunkPosition(int chunkX, int chunkZ);
	int chunkX() const;
	int chunkZ() const;

	// Цвет рамки
	void setBorderColor(const QColor& color);

	QColor borderColor() const;

	// zLevel
	void setZLevel(float level);

	float zLevel() const;

	/// Установить тайлсет
	void setTileset(std::shared_ptr<TextureAtlas> tileset);

	/// Установить размер чанка
	void setChunkSize(const QSize& size);
	QSize chunkSize() const;

	/// Установить тайл по локальным координатам (0..chunkSize-1)
	void setTile(int worldX, int worldZ, int tileId, int layer);

	/// Получить тайл по локальным координатам
	int getTile(int worldX, int worldZ, int layer) const;

	/// Перестроить VBO из данных тайлов
	void rebuild();

	/// Отрисовка чанка
	void render();

	/// Отрисовка рамки чанка (всегда доступна, управление на уровне TileRenderer)
	void renderBorder();

	float worldMinX() const;
	float worldMaxX() const;
	float worldMinZ() const;
	float worldMaxZ() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
