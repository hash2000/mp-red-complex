#pragma once
#include <QString>
#include <QPixmap>

enum class ItemResourceType {
	Sand,
	Soil,
	Clay,
	Stone,
	Granite,
	Basalt,
	Limestone,
	Coal,
	IronOre,
	GoldVein,
	OilShale,
	Aquifer,
	GasPocket,
	DiamondPipe,
	TitaniumDeposit,
};

enum class ItemType {
	Equipment, // Экипировка (оружие, броня)
	Resource, // Сырьё (ресурсы для крафта)
	Component, // Запчасти (сложные элементы из сырья)
	Container, // Ящики (могут содержать другие предметы)
	Recipe, // Рецепт
};

enum class ItemRarityType {
	Common,
	Improved,
	Stable,
	Rare,
	Unique,
};

// если этот предмет-экиперовка, сразу нежно знать куда эта экиперовка одевается
enum class ItemEquipmentType {
	Head,
	Body,
	Weapon,
	Shield,
	Gloves,
	Boots,
	Ring,
	Amulet,
	Resource,
	Consumable,
};

// рецепты
class  ItemRecipe {
public:
	struct Ingredient {
		QString itemId;
		int amount;
	};
	std::vector<Ingredient> ingredients;
};

class ItemContainer {
public:
	int rows = 1;
	int cols = 1;
};

class ItemEntity {
public:
	QString id;
	QString name;
	ItemType type;
	QString description;
	QString iconPath;
	QPixmap icon;

	int width = 1;
	int height = 1;
	// стек 
	int maxStack = 1;

	ItemRarityType rarity = ItemRarityType::Common;
	// если это экипировка, то какая
	std::optional<ItemEquipmentType> equipmentType;
	// рецепт
	std::optional<ItemRecipe> recipe;
	// контейнер
	std::optional<ItemContainer> container;
};

class Item {
public:
	QString id;
	QString entityId;
	const ItemEntity* entity;
};
