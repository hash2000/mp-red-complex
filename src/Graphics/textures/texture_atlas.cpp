#include "Graphics/textures/texture_atlas.h"
#include <QImage>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
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
	bool useMipMaps = true;
	bool useMipMapsSmoothing = true;
	TextureFilter textureFilter = TextureFilter::Linear;
	std::unique_ptr<QOpenGLTexture> texture;
};

TextureAtlas::TextureAtlas(int tileSizeX, int tileSizeY)
	: d(std::make_unique<Private>(this, tileSizeX, tileSizeY)) {
}

TextureAtlas::~TextureAtlas() = default;

void TextureAtlas::useMipMaps(bool use) {
	d->useMipMaps = use;
}

void TextureAtlas::useMipMapsSmoothing(bool use) {
	d->useMipMapsSmoothing = use;
}

void TextureAtlas::setTextureFilter(TextureFilter filter) {
	d->textureFilter = filter;
}

bool TextureAtlas::loadFromPixmap(const QPixmap& pixmap, int tilesCountX, int tilesCountY) {
	if (pixmap.isNull()) {
		qWarning() << "TextureAtlas: pixmap is null";
		return false;
	}

	d->tilesCountX = tilesCountX;
	d->tilesCountY = tilesCountY;
	d->tileSizeX = pixmap.width() / tilesCountX;
	d->tileSizeY = pixmap.height() / tilesCountY;

	// 1. Конвертируем в QImage и переворачиваем по Y (OpenGL: (0,0) — нижний левый)
	QImage image = pixmap.toImage().mirrored(false, true);

	// 2. 🔥 ОБЯЗАТЕЛЬНО: конвертируем в формат, понятный OpenGL
	if (image.format() != QImage::Format_RGBA8888) {
		image = image.convertToFormat(QImage::Format_RGBA8888);
	}

	qInfo() << "TextureAtlas::loadFromPixmap - image format:" << image.format()
		<< "size:" << image.size();

	// 3. Создаём текстуру ЯВНО, с контролем формата
	d->texture = std::make_unique<QOpenGLTexture>(image);

	if (!d->texture->isCreated()) {
		qWarning() << "TextureAtlas: failed to create OpenGL texture";
		return false;
	}

	d->texture->setWrapMode(QOpenGLTexture::ClampToEdge);

	if (d->useMipMaps) {
		d->texture->generateMipMaps();
		if (d->textureFilter == TextureFilter::Nearest) {
			d->texture->setMinificationFilter(QOpenGLTexture::NearestMipMapNearest);
		}
		else {
			d->texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
		}
		if (d->useMipMapsSmoothing) {
			d->texture->setMipBaseLevel(0);
			d->texture->setMipMaxLevel(d->texture->mipLevels() - 1);
		}
	}
	else {

		if (d->textureFilter == TextureFilter::Nearest) {
			d->texture->setMinificationFilter(QOpenGLTexture::Nearest);
		}
		else {
			d->texture->setMinificationFilter(QOpenGLTexture::Linear);
		}
	}

	d->texture->setMagnificationFilter(QOpenGLTexture::Linear);

	qInfo() << "TextureAtlas loaded:" << pixmap.size()
		<< "tiles:" << tilesCountX << "x" << tilesCountY
		<< "tileSize:" << d->tileSizeX << "x" << d->tileSizeY
		<< "glId:" << d->texture->textureId()
		<< "mipLevels:" << d->texture->mipLevels();

	return true;
}

TextureRegion TextureAtlas::getRegion(int tileX, int tileY) const {
	if (!d->texture || tileX < 0 || tileX >= d->tilesCountX || tileY < 0 || tileY >= d->tilesCountY) {
		qWarning() << "TextureAtlas: invalid tile coordinates" << tileX << tileY;
		return TextureRegion();
	}

	float u1 = static_cast<float>(tileX) / d->tilesCountX;
	float u2 = static_cast<float>(tileX + 1) / d->tilesCountX;
	float v1 = 1.0f - static_cast<float>(tileY + 1) / d->tilesCountY;
	float v2 = 1.0f - static_cast<float>(tileY) / d->tilesCountY;

	return TextureRegion(u1, v1, u2, v2);
}

void TextureAtlas::bind() const {
	if (d->texture) {
		if (!d->texture->isCreated()) {
			qWarning() << "Texture is not created";
			return;
		}

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
