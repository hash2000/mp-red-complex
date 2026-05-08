#pragma once
#include "Graphics/textures/tile_data.h"
#include <QPoint>
#include <QSize>
#include <QColor>
#include <memory>

class ChunkDrawData;
class Material;
class RenderQueue;

/// Чанк - блок тайлов NxN с собственным VAO/VBO
/// Координаты: XZ плоскость, Y = высота
class Chunk {
public:
	/// Размер чанка по умолчанию
	static constexpr int kDefaultChunkSize = 32;

	Chunk();
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

	/// Установить размер чанка
	void setChunkSize(const QSize& size);
	QSize chunkSize() const;

	/// Перестроить VBO из данных тайлов
	void rebuild();

	/// Отрисовка чанка через очередь команд
	void render(RenderQueue& queue);

	/// Отрисовка рамки чанка (всегда доступна, управление на уровне TileRenderer)
	void renderBorder(RenderQueue& queue);

	void showOnlyOneLayer(int layer);
	void showAllLayers();

	float worldMinX() const;
	float worldMaxX() const;
	float worldMinZ() const;
	float worldMaxZ() const;

	int layerCount() const;
	int addLayer(std::unique_ptr<ChunkDrawData>&& layer, Material* material = nullptr);
	void removeLayer(int layer);

	/// Получить материал для слоя
	Material* layerMaterial(int layer) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
