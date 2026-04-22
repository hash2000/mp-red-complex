#pragma once
#include "DataLayer/images/i_images_data_provider.h"
#include <QObject>
#include <memory>
#include <QHash>
#include <QPixmap>
#include <optional>

// Стандартный тег для постоянного кеша (UI элементы)
inline constexpr const char* kDefaultTextureTag = "default";
inline constexpr const char* kTestTexturePath = "test-image-stub";

class ImagesService : public QObject {
	Q_OBJECT
public:
	explicit ImagesService(
		IImagesDataProvider* dataProvider,
		QObject* parent = nullptr);

	~ImagesService() override;

	// Получение текстуры (с кэшированием в указанный тег)
	QPixmap getImage(const QString& path, ImageType type = ImageType::Icon, const QString& tag = kDefaultTextureTag);

	// Очистка всего кеша
	void clearCache();

	// Очистка кеша по тегу
	void clearCacheByTag(const QString& tag);

	// Предварительная загрузка текстуры в кэш (с указанием тега)
	void preloadImage(const QString& name, ImageType type = ImageType::Icon, const QString& tag = kDefaultTextureTag);

	// Получить список всех доступных текстур для типа
	QStringList listImages(ImageType type) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
