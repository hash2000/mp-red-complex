#pragma once
#include <QString>
#include <QPixmap>
#include <QUuid>
#include <list>

enum class ItemResourceType {
	Ore, // Руда
	Chemical, // Химикаты
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
	None,
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
	Backpack,
	Bag,

	LastSlot,
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

	struct {
		struct {
			std::list<ItemResourceType> all;
			std::list<ItemResourceType> any;
		} resources;
	} permissions;
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
	ItemEquipmentType equipmentType;

	// рецепт
	std::optional<ItemRecipe> recipe;

	// контейнер
	std::optional<ItemContainer> container;

	// тип ресурса
	std::list<ItemResourceType> resourceType;
};

class Item {
public:
	QUuid id;
	QString entityId;

	const ItemEntity* entity;
};
