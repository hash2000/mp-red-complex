#include "Graphics/textures/uploaded_texture.h"
#include <QImage>
#include <QOpenGLTexture>
#include <QOpenGLFunctions>
#include <QOpenGLFunctions_3_3_Core>
#include <QPixmap>
#include <qdebug.h>

class UploadedTexture::Private {
public:
	Private(UploadedTexture* parent) : q(parent) {
	}

	UploadedTexture* q;
	int width = 0;
	int height = 0;
	UploadedTextureProperties properties;
	std::unique_ptr<QOpenGLTexture> texture;
};

UploadedTexture::UploadedTexture()
: d(std::make_unique<Private>(this)) {
}

UploadedTexture::~UploadedTexture() = default;


bool UploadedTexture::loadFromPixmap(const QPixmap& pixmap, const UploadedTextureProperties& properties) {
	if (pixmap.isNull()) {
		qWarning() << "UploadedTexture::loadFromPixmap: pixmap is null";
		return false;
	}

	d->properties = properties;
	d->width = pixmap.width();
	d->height = pixmap.height();

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

	if (d->properties.useMipMaps) {
		d->texture->generateMipMaps();
		if (d->properties.textureFilter == TextureFilter::Nearest) {
			d->texture->setMinificationFilter(QOpenGLTexture::NearestMipMapNearest);
		}
		else {
			d->texture->setMinificationFilter(QOpenGLTexture::LinearMipMapLinear);
		}
		if (d->properties.useMipMapsSmoothing) {
			d->texture->setMipBaseLevel(0);
			d->texture->setMipMaxLevel(d->texture->mipLevels() - 1);
		}
	}
	else {

		if (d->properties.textureFilter == TextureFilter::Nearest) {
			d->texture->setMinificationFilter(QOpenGLTexture::Nearest);
		}
		else {
			d->texture->setMinificationFilter(QOpenGLTexture::Linear);
		}
	}

	d->texture->setMagnificationFilter(QOpenGLTexture::Linear);

	qInfo() << "UploadedTexture::loadFromPixmap:" << pixmap.size()
		<< "sizes:" << d->width << "x" << d->height
		<< "glId:" << d->texture->textureId()
		<< "mipLevels:" << d->texture->mipLevels();

	return true;

}

void UploadedTexture::bind() const {
	if (d->texture) {
		if (!d->texture->isCreated()) {
			qWarning() << "UploadedTexture::bind: Texture is not created";
			return;
		}

		d->texture->bind();
	}
}

void UploadedTexture::unbind() const {
	if (d->texture) {
		d->texture->release();
	}
}

unsigned int  UploadedTexture::textureId() const {
	return d->texture ? d->texture->textureId() : 0;
}

bool UploadedTexture::isLoaded() const {
	return d->texture != nullptr;
}

void UploadedTexture::unload() {
	if (d->texture) {
		d->texture.reset();
	}
}

int UploadedTexture::width() const {
	return d->width;
}

int UploadedTexture::height() const {
	return d->height;
}
