#pragma once
#include "Graphics/textures/texture_region.h"
#include <QOpenGLFunctions_3_3_Core>
#include <memory>

class QOpenGLTexture;
class QPixmap;

/// Тексурный атлас - загружает тайлсет в одну большую OpenGL текстуру
/// и рассчитывает UV координаты для каждого тайла
class TextureAtlas {
public:
	enum class TextureFilter {
		Linear,
		Nearest,
	};

	/// Конструктор
	/// @param tileSizeX Ширина одного тайла в пикселях
	/// @param tileSizeY Высота одного тайла в пикселях
	TextureAtlas(int tileSizeX, int tileSizeY);
	~TextureAtlas();

	/// Загрузить тайлсет из изображения
	/// @param pixmap Изображение тайлсета
	/// @param tilesCountX Количество тайлов по X в тайлсете
	/// @param tilesCountY Количество тайлов по Y в тайлсете
	bool loadFromPixmap(const QPixmap& pixmap, int tilesCountX, int tilesCountY);

	/// Получить UV区域 для конкретного тайла
	/// @param tileX Позиция тайла по X (0..tilesCountX-1)
	/// @param tileY Позиция тайла по Y (0..tilesCountY-1)
	TextureRegion getRegion(int tileX, int tileY) const;

	void useMipMaps(bool use);

	void useMipMapsSmoothing(bool use);

	void setTextureFilter(TextureFilter filter);

	/// Забиндить текстуру для рендеринга
	void bind() const;

	/// Отбиндить текстуру
	void unbind() const;

	/// ID OpenGL текстуры
	GLuint textureId() const;

	/// Размеры
	int tileSizeX() const;
	int tileSizeY() const;
	int tilesCountX() const;
	int tilesCountY() const;

	bool isLoaded() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
