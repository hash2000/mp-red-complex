#pragma once
#include "Graphics/tiles/tile_data.h"
#include <QOpenGLFunctions_3_3_Core>
#include <QPoint>
#include <QSize>
#include <memory>

class QOpenGLVertexArrayObject;
class QOpenGLBuffer;
class Tileset;

/// Чанк - блок тайлов NxN с собственным VAO/VBO
/// Координаты: XZ плоскость, Y = высота
class Chunk : protected QOpenGLFunctions_3_3_Core {
public:
	/// Размер чанка по умолчанию
	static constexpr int kDefaultChunkSize = 32;

	Chunk();
	~Chunk();

	/// Установить позицию чанка в мире (в чанках, не в пикселях)
	void setChunkPosition(int chunkX, int chunkZ);
	int chunkX() const;
	int chunkZ() const;

	/// Установить тайлсет
	void setTileset(Tileset* tileset);

	/// Установить размер чанка
	void setChunkSize(const QSize& size);

	/// Установить тайл по локальным координатам (0..chunkSize-1)
	void setTile(int worldX, int worldZ, int tileId);

	/// Получить тайл по локальным координатам
	int getTile(int worldX, int worldZ) const;

	/// Пометить как грязный (требует перестройки VBO)
	void markDirty();
	bool isDirty() const;

	/// Перестроить VBO из данных тайлов
	void rebuild();

	/// Отрисовка чанка
	void render();

	/// Отрисовка рамки чанка (всегда доступна, управление на уровне TileRenderer)
	void renderBorder();

	/// Проверка инициализации
	bool isInitialized() const;

	/// Границы чанка в мировых координатах (для frustum culling)
	float worldMinX() const;
	float worldMaxX() const;
	float worldMinZ() const;
	float worldMaxZ() const;

private:
	/// Перестроить VBO рамки
	void rebuildBorderVBO();
	void rebuildVertexes();

	/// Инициализация (вызвать после создания OpenGL контекста)
	void initialize();

	class Private;
	std::unique_ptr<Private> d;
};
