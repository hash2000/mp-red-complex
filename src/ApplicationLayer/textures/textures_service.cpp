#include "ApplicationLayer/textures/textures_service.h"
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QPixmap>
#include <QHash>

class TexturesService::Private {
public:
	Private(TexturesService* parent)
		: q(parent) {
	}

	TexturesService* q;
	ITexturesDataProvider* dataProvider = nullptr;

	// Тегированный кэш загруженных текстур
	// Внешний QHash: ключ = тег, значение = внутренний QHash
	// Внутренний QHash: ключ = путь к текстуре, значение = QPixmap
	QHash<QString, QHash<QString, QPixmap>> texturesCache;
};

TexturesService::TexturesService(
	ITexturesDataProvider* dataProvider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
}

TexturesService::~TexturesService() = default;

QPixmap TexturesService::getTexture(const QString& path, TextureType type, const QString& tag) {
	// Проверяем кэш по тегу
	auto tagIt = d->texturesCache.find(tag);
	if (tagIt != d->texturesCache.end()) {
		auto pathIt = tagIt->find(path);
		if (pathIt != tagIt->end()) {
			return *pathIt;
		}
	}

	// Загружаем из провайдера
	auto pixmap = d->dataProvider->loadTexture(path, type);
	if (pixmap.has_value()) {
		// Сохраняем в кеш с указанным тегом
		d->texturesCache[tag].insert(path, *pixmap);
		return *pixmap;
	}

	// Возвращаем пустой pixmap, если загрузка не удалась
	return QPixmap();
}

void TexturesService::clearCache() {
	d->texturesCache.clear();
}

void TexturesService::clearCacheByTag(const QString& tag) {
	d->texturesCache.remove(tag);
}

void TexturesService::preloadTexture(const QString& name, TextureType type, const QString& tag) {
	// Проверяем, есть ли уже в кеше по этому тегу
	auto tagIt = d->texturesCache.find(tag);
	if (tagIt != d->texturesCache.end() && tagIt->contains(name)) {
		return; // Уже в кеше
	}

	// Загружаем и сохраняем в кеш с указанным тегом
	auto pixmap = d->dataProvider->loadTexture(name, type);
	if (pixmap.has_value()) {
		d->texturesCache[tag].insert(name, *pixmap);
	}
}

QStringList TexturesService::listTextures(TextureType type) const {
	return d->dataProvider->listTextures(type);
}
