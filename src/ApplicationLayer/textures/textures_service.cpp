#include "ApplicationLayer/textures/textures_service.h"
#include "ApplicationLayer/textures/images_service.h"
#include "Graphics/textures/uploaded_texture.h"
#include <QHash>

class TexturesService::Private {
public:
	Private(TexturesService* parent) : q(parent) {
	}

	TexturesService* q;
	ImagesService* imagesService;
	QHash<QString, QHash<QString, std::shared_ptr<UploadedTexture>>> texturesCache;
};


TexturesService::TexturesService(ImagesService* imagesService)
	: d(std::make_unique<Private>(this)) {
	d->imagesService = imagesService;
}

TexturesService::~TexturesService() = default;

std::shared_ptr<UploadedTexture> TexturesService::upload(
	const QString& path,
	const UploadedTextureProperties& properties,
	ImageType type,
	const QString& tag) {

	auto pixmap = d->imagesService->getImage(path, type, tag);
	if (pixmap.isNull()) {
		return std::shared_ptr<UploadedTexture>();
	}

	auto texture = std::make_shared<UploadedTexture>();
	if (!texture->loadFromPixmap(pixmap, properties)) {
		return std::shared_ptr<UploadedTexture>();
	}

	return texture;
}
