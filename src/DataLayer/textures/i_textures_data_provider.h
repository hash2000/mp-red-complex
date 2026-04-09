#pragma once
#include <QString>
#include <QStringList>
#include <QPixmap>
#include <optional>

// Метаданные тайлового набора
struct TileSetMetadata {
	struct Size {
		int x = 64;
		int y = 64;
	};
	Size gridSize;
};

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

	// Загрузить метаданные тайлового набора (если type == TileSets)
	virtual std::optional<TileSetMetadata> loadTileSetMetadata(const QString& path) const = 0;
};
