#pragma once
#include "DataLayer/textures/i_textures_data_provider.h"
#include <QObject>
#include <memory>
#include <QHash>
#include <QPixmap>
#include <optional>

// Стандартный тег для постоянного кеша (UI элементы)
inline constexpr const char* kDefaultTextureTag = "default";
inline constexpr const char* kTestTexturePath = "test-texture-stub";

class TexturesService : public QObject {
	Q_OBJECT
public:
	explicit TexturesService(
		ITexturesDataProvider* dataProvider,
		QObject* parent = nullptr);
	~TexturesService() override;

	// Получение текстуры (с кэшированием в указанный тег)
	QPixmap getTexture(const QString& path, TextureType type = TextureType::Icon, const QString& tag = kDefaultTextureTag);

	// Очистка всего кеша
	void clearCache();

	// Очистка кеша по тегу
	void clearCacheByTag(const QString& tag);

	// Предварительная загрузка текстуры в кэш (с указанием тега)
	void preloadTexture(const QString& name, TextureType type = TextureType::Icon, const QString& tag = kDefaultTextureTag);

	// Получить список всех доступных текстур для типа
	QStringList listTextures(TextureType type) const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
