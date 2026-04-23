#include "ApplicationLayer/textures/images_service.h"
#include "DataLayer/images/i_images_data_provider.h"
#include <QPixmap>
#include <QPainter>
#include <QHash>

class ImagesService::Private {
public:
	Private(ImagesService* parent)
		: q(parent) {
	}

	ImagesService* q;
	IImagesDataProvider* dataProvider = nullptr;

	// Тегированный кэш загруженных текстур
	// Внешний QHash: ключ = тег, значение = внутренний QHash
	// Внутренний QHash: ключ = путь к текстуре, значение = QPixmap
	QHash<QString, QHash<QString, QPixmap>> imagesCache;

	QPixmap generateTestPixmap() {
		QPixmap pixmap(1024, 1024);
		pixmap.fill(Qt::gray);

		QPainter painter(&pixmap);
		for (int y = 0; y < 32; y++) {
			for (int x = 0; x < 32; x++) {
				QColor color = QColor::fromHsv(((x + y) % 18) * 20, 150, 200);
				painter.fillRect(x * 32, y * 32, 32, 32, color);
			}
		}

		painter.end();
		return pixmap;
	}
};

ImagesService::ImagesService(
	IImagesDataProvider* dataProvider,
	QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
	d->imagesCache[kDefaultImageTag][kStubImagePath] = d->generateTestPixmap();
}

ImagesService::~ImagesService() = default;

QPixmap ImagesService::getImage(const QString& path, ImageType type, const QString& tag) {
	// Проверяем кэш по тегу
	auto tagIt = d->imagesCache.find(tag);
	if (tagIt != d->imagesCache.end()) {
		auto pathIt = tagIt->find(path);
		if (pathIt != tagIt->end()) {
			return *pathIt;
		}
	}

	// Загружаем из провайдера
	auto pixmap = d->dataProvider->load(path, type);
	if (pixmap.has_value()) {
		// Сохраняем в кеш с указанным тегом
		d->imagesCache[tag].insert(path, *pixmap);
		return *pixmap;
	}

	// Возвращаем пустой pixmap, если загрузка не удалась
	return QPixmap();
}

void ImagesService::clearCache() {
	d->imagesCache.clear();
}

void ImagesService::clearCacheByTag(const QString& tag) {
	d->imagesCache.remove(tag);
}

void ImagesService::preloadImage(const QString& name, ImageType type, const QString& tag) {
	// Проверяем, есть ли уже в кеше по этому тегу
	auto tagIt = d->imagesCache.find(tag);
	if (tagIt != d->imagesCache.end() && tagIt->contains(name)) {
		return; // Уже в кеше
	}

	// Загружаем и сохраняем в кеш с указанным тегом
	auto pixmap = d->dataProvider->load(name, type);
	if (pixmap.has_value()) {
		d->imagesCache[tag].insert(name, *pixmap);
	}
}

QStringList ImagesService::listImages(ImageType type) const {
	return d->dataProvider->list(type);
}
