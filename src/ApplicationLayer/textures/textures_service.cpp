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

	// Кэш загруженных текстур
	QHash<QString, QPixmap> texturesCache;
};

TexturesService::TexturesService(
	ITexturesDataProvider* dataProvider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
}

TexturesService::~TexturesService() = default;

QPixmap TexturesService::getTexture(const QString& path) {
	// Проверяем кэш
	auto it = d->texturesCache.find(path);
	if (it != d->texturesCache.end()) {
		return *it;
	}

	// Загружаем из провайдера
	auto pixmap = d->dataProvider->loadTexture(path);
	if (pixmap.has_value()) {
		d->texturesCache.insert(path, *pixmap);
		return *pixmap;
	}

	// Возвращаем пустой pixmap, если загрузка не удалась
	return QPixmap();
}

void TexturesService::clearCache() {
	d->texturesCache.clear();
}

void TexturesService::preloadTexture(const QString& name) {
	// Предварительная загрузка в кэш (без возврата значения)
	if (!d->texturesCache.contains(name)) {
		auto pixmap = d->dataProvider->loadIcon(name);
		if (pixmap.has_value()) {
			d->texturesCache.insert(name, *pixmap);
		}
	}
}
