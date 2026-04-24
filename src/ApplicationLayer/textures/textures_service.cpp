#include "ApplicationLayer/textures/textures_service.h"
#include "ApplicationLayer/textures/images_service.h"
#include "Graphics/textures/uploaded_texture.h"
#include <QHash>
#include <QDebug>

struct TextureKey {
	QString path;
	ImageType type = ImageType::Icon;
	UploadedTextureProperties properties;

	bool operator==(const TextureKey& other) const {
		return path == other.path &&
			type == other.type &&
			properties == other.properties;
	}
};


inline uint qHash(const TextureKey& key, uint seed = 0) {
	return
		qHash(key.path, seed) ^
		qHash(static_cast<int>(key.type), seed) ^
		qHash(key.properties, seed);
}

class TexturesService::Private {
public:
	Private(TexturesService* parent) : q(parent) {
	}

	TexturesService* q;
	ImagesService* imagesService;
	QHash<QString, QHash<uint, std::shared_ptr<UploadedTexture>>> texturesCache;
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

	TextureKey key{path, type, properties};
	auto keyHash = qHash(key);

	// Проверяем кэш по тегу
	auto tagIt = d->texturesCache.find(tag);
	if (tagIt != d->texturesCache.end()) {
		auto textureIt = tagIt->find(keyHash);
		if (textureIt != tagIt->end()) {
			qDebug() << "TexturesService: texture found in cache, tag:" << tag << "path:" << path;
			return *textureIt;
		}
	}

	// Загружаем пиксельную карту через ImagesService
	auto pixmap = d->imagesService->getImage(path, type, tag);
	if (pixmap.isNull()) {
		qWarning() << "TexturesService: failed to load image, path:" << path << "type:" << static_cast<int>(type);
		return std::shared_ptr<UploadedTexture>();
	}

	// Создаём текстуру и загружаем в видеопамять
	auto texture = std::make_shared<UploadedTexture>();
	if (!texture->loadFromPixmap(pixmap, properties)) {
		qWarning() << "TexturesService: failed to upload texture to GPU, path:" << path;
		return std::shared_ptr<UploadedTexture>();
	}

	// Сохраняем в кэше
	d->texturesCache[tag][keyHash] = texture;
	qDebug() << "TexturesService: texture uploaded and cached, tag:" << tag << "path:" << path << "textureId:" << texture->textureId();

	return texture;
}

void TexturesService::clear(const QString& tag) {
	auto tagIt = d->texturesCache.find(tag);
	if (tagIt == d->texturesCache.end()) {
		return;
	}

	for (auto it = tagIt->begin(); it != tagIt->end(); it++) {
		auto& texture = it.value();
		texture->unload();
	}

	d->texturesCache.erase(tagIt);
}
