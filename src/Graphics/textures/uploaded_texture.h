#pragma once
#include "Graphics/textures/texture_filter.h"
#include <QPixmap>
#include <memory>

struct UploadedTextureProperties {
	bool useMipMaps = true;
	bool useMipMapsSmoothing = true;
	TextureFilter textureFilter = TextureFilter::Linear;
};

class UploadedTexture {
public:
	UploadedTexture();
	~UploadedTexture();

	bool loadFromPixmap(const QPixmap& pixmap, const UploadedTextureProperties& properties);

	// Забиндить текстуру для рендеринга
	void bind() const;

	// Отбиндить текстуру
	void unbind() const;

	// ID OpenGL текстуры
	unsigned int textureId() const;

	// Загружена текстура или нет
	bool isLoaded() const;

	// Выгрузить текстуру
	void unload();

	int width() const;

	int height() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
