#include "Graphics/textures/texture_atlas.h"
#include <QImage>
#include <QOpenGLTexture>
#include <QPixmap>
#include <qdebug.h>

class TextureAtlas::Private {
public:
	Private(TextureAtlas* parent, int tileSizeX, int tileSizeY)
		: q(parent)
		, tileSizeX(tileSizeX)
		, tileSizeY(tileSizeY) {
	}

	TextureAtlas* q;
	int tileSizeX;
	int tileSizeY;
	int tilesCountX = 0;
	int tilesCountY = 0;
	std::unique_ptr<QOpenGLTexture> texture;
};

TextureAtlas::TextureAtlas(int tileSizeX, int tileSizeY)
	: d(std::make_unique<Private>(this, tileSizeX, tileSizeY)) {
}

TextureAtlas::~TextureAtlas() = default;

bool TextureAtlas::loadFromPixmap(const QPixmap& pixmap, int tilesCountX, int tilesCountY) {
	if (pixmap.isNull()) {
		qWarning() << "TextureAtlas: pixmap is null";
		return false;
	}

	d->tilesCountX = tilesCountX;
	d->tilesCountY = tilesCountY;

	// Конвертируем в QImage для переворота
	QImage image = pixmap.toImage().mirrored(false, true);

	// Создаем OpenGL текстуру
	d->texture = std::make_unique<QOpenGLTexture>(image);
	d->texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
	d->texture->setMagnificationFilter(QOpenGLTexture::Linear);
	d->texture->setWrapMode(QOpenGLTexture::ClampToEdge);

	qInfo() << "TextureAtlas loaded:" << pixmap.width() << "x" << pixmap.height()
		<< "tiles:" << tilesCountX << "x" << tilesCountY
		<< "tile size:" << d->tileSizeX << "x" << d->tileSizeY;

	return true;
}

TextureRegion TextureAtlas::getRegion(int tileX, int tileY) const {
	if (!d->texture || tileX < 0 || tileX >= d->tilesCountX || tileY < 0 || tileY >= d->tilesCountY) {
		qWarning() << "TextureAtlas: invalid tile coordinates" << tileX << tileY;
		return TextureRegion();
	}

	float tileWidth = static_cast<float>(d->tileSizeX) / d->texture->width();
	float tileHeight = static_cast<float>(d->tileSizeY) / d->texture->height();

	float u1 = tileX * tileWidth;
	float v1 = tileY * tileHeight;
	float u2 = u1 + tileWidth;
	float v2 = v1 + tileHeight;

	return TextureRegion(u1, v1, u2, v2);
}

void TextureAtlas::bind() const {
	if (d->texture) {
		d->texture->bind();
	}
}

void TextureAtlas::unbind() const {
	if (d->texture) {
		d->texture->release();
	}
}

GLuint TextureAtlas::textureId() const {
	return d->texture ? d->texture->textureId() : 0;
}

int TextureAtlas::tileSizeX() const {
	return d->tileSizeX;
}

int TextureAtlas::tileSizeY() const {
	return d->tileSizeY;
}

int TextureAtlas::tilesCountX() const {
	return d->tilesCountX;
}

int TextureAtlas::tilesCountY() const {
	return d->tilesCountY;
}

bool TextureAtlas::isLoaded() const {
	return d->texture != nullptr;
}
