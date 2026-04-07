#pragma once
#include "Graphics/tiles/chunk.h"
#include <QOpenGLFunctions_3_3_Core>
#include <memory>

class Camera;
class QOpenGLShaderProgram;
class Tileset;

/// Рендерер тайлов - управляет чанками и их отрисовкой
class TileRenderer : protected QOpenGLFunctions_3_3_Core {
public:
	TileRenderer();
	~TileRenderer();

	/// Инициализация (вызвать после создания OpenGL контекста)
	bool initialize();

	/// Установить тайлсет
	void setTileset(Tileset* tileset);

	/// Получить или создать чанк по координатам
	Chunk* getOrCreateChunk(int chunkX, int chunkZ);

	/// Удалить чанк
	void removeChunk(int chunkX, int chunkZ);

	/// Удалить все чанки
	void clearChunks();

	/// Перестроить все грязные чанки
	void rebuildDirtyChunks();

	/// Отрисовка всех видимых чанков
	void render(const Camera& camera, int viewportWidth, int viewportHeight);

	/// Уровень высоты (Y), на котором рисуются тайлы
	float zLevel() const;
	void setZLevel(float level);

	/// Количество загруженных чанков
	size_t chunkCount() const;

private:
	/// Проверка видимости чанка во frustum камеры
	bool isChunkVisible(const Chunk* chunk, const Camera& camera, int viewportWidth, int viewportHeight) const;

	class Private;
	std::unique_ptr<Private> d;
};
