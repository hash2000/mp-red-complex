#pragma once
#include <QString>
#include <QPixmap>
#include <QUuid>

#include <optional>
#include <list>
#include <vector>

enum class ItemResourceType {
	Undefined,
	Ore, // Руда
	Chemical, // Химикаты
};

enum class ItemType {
	Undefined,
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

// Подтип указывает куда может быть установлен предмет
enum class ItemSubType {
	Undefined,
	//-----------
	// Экипировка
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
	//-----------
	Root,

	LastSlot,
};

// Слот куда конкретно установлен предмет в контейнере
enum class ItemSlotType {
	//-----------
	// Экипировка
	Head,
	Body,
	WeaponLeft,
	WeaponRight,
	Gloves,
	Amulet,
	Boots,
	Ring1,
	Ring2,
	Ring3,
	Backpack,
	Bag1,
	Bag2,
	//-----------
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

struct ItemContainerPermissions {
	struct {
		std::list<ItemResourceType> all;
		std::list<ItemResourceType> any;
	} resources;
};

class ItemContainer {
public:
	int rows = 1;
	int cols = 1;

	ItemContainerPermissions permissions;
};

class ItemEntity {
public:
	QString id;
	QString name;
	ItemType type;
	ItemSubType subType;
	QString description;
	QString iconPath;
	QPixmap icon;

	int width = 1;
	int height = 1;

	int maxStack = 1;

	ItemRarityType rarity = ItemRarityType::Common;

	std::optional<ItemRecipe> recipe;
	std::optional<ItemContainer> container;
	std::list<ItemResourceType> resourceType;
};

class Item {
public:
	QUuid id;
	QUuid containerId;
	QString entityId;
	int level = 1;
	int count = 1;

	struct {
		int base;
		int secondary;
	} position;

	std::shared_ptr<ItemEntity> entity;
};
