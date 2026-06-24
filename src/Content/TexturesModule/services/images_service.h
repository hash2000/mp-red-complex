#pragma once
#include <QObject>
#include <memory>
#include <QHash>
#include <QPixmap>
#include <optional>

// Стандартный тег для постоянного кеша (UI элементы)
inline constexpr const char* kDefaultImageTag = "default";
inline constexpr const char* kStubImagePath = "image-stub";

class IImagesDataProvider;
enum class ImageType;

class ImagesService : public QObject {
	Q_OBJECT
public:
	explicit ImagesService(
		IImagesDataProvider* dataProvider,
		QObject* parent = nullptr);

	~ImagesService() override;

	// Получение текстуры (с кэшированием в указанный тег)
	QPixmap getImage(const QString& path, ImageType typ, const QString& tag = kDefaultImageTag);

	// Очистка всего кеша
	void clearCache();

	// Очистка кеша по тегу
	void clearCacheByTag(const QString& tag);

	// Предварительная загрузка текстуры в кэш (с указанием тега)
	void preloadImage(const QString& name, ImageType type, const QString& tag = kDefaultImageTag);

	// Получить список всех доступных текстур для типа
	QStringList listImages(ImageType type) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
