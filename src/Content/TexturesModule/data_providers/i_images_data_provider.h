#pragma once
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <optional>

enum class ImageType {
	Icon,       // assets/icons/
	Entity,     // assets/entities/
	Item,       // assets/items/
	Character,  // assets/characters/
	Equipment,  // assets/equipment/
	Users,			// assets/users/
	TileSets,		// assets/textures/tiles/
	LastSlot,   // Маркер конца (для итерации)
};

class IImagesDataProvider {
public:
	virtual ~IImagesDataProvider() = default;

	// Загрузка текстуры по пути с указанием типа
	virtual std::optional<QPixmap> load(const QString& path, ImageType type = ImageType::Icon) const = 0;

	// Получить список всех доступных PNG текстур для указанного типа
	virtual QStringList list(ImageType type) const = 0;
};
