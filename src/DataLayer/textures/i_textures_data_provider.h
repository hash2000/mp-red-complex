#pragma once
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <optional>

enum class TextureType {
	Icon,       // assets/icons/
	Entity,     // assets/entities/
	Item,       // assets/items/
	Character,  // assets/characters/
	Equipment,  // assets/equipment/
	Users,			// assets/users/
	TileSets,		// assets/textures/tiles/
	LastSlot,   // Маркер конца (для итерации)
};

class ITexturesDataProvider {
public:
	virtual ~ITexturesDataProvider() = default;

	// Загрузка текстуры по пути с указанием типа
	virtual std::optional<QPixmap> loadTexture(const QString& path, TextureType type = TextureType::Icon) const = 0;

	// Получить список всех доступных PNG текстур для указанного типа
	virtual QStringList listTextures(TextureType type) const = 0;
};
