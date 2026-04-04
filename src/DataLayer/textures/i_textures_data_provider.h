#pragma once
#include <QString>
#include <QPixmap>
#include <optional>

enum class TextureType {
	Icon,       // assets/icons/
	Entity,     // assets/entities/
	Item,       // assets/items/
	Character,  // assets/characters/
	Equipment,  // assets/equipment/
	Users,			// assets/users/
	LastSlot,   // Маркер конца (для итерации)
};

class ITexturesDataProvider {
public:
	virtual ~ITexturesDataProvider() = default;

	// Загрузка текстуры по пути с указанием типа
	virtual std::optional<QPixmap> loadTexture(const QString& path, TextureType type = TextureType::Icon) const = 0;
};
