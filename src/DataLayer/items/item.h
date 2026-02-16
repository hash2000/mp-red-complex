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

enum class ItemEquipmentType {
	Head,
	Body,
	Weapon,
	Shield,
	Gloves,
	Boots,
	Ring,
	Amulet,
	Consumable,
	Resource,
};

enum class ItemEquipmentSlotType {
	Head,
	Body,
	WeaponLeft,
	WeaponRight,
	GlovesLeft,
	GlovesRight,
	Boots,
	RingLeft,
	RingRight,
	Amulet,
};

// рецепты
struct ItemRecipe {
	struct Ingredient {
		QString itemId;
		int amount;
	};
	std::vector<Ingredient> ingredients;
};

struct Item {
	QString id;
	QString name;
	ItemType type;
	QString description;
	QString iconPath;
	QPixmap icon;

	// идентификатор элемента
	QString entityId;
	// тжентификатор инвентаря, может находиться только в одном инвентаре
	std::optional<QString> inventoryId;

	// позиция в инвентаре
	int x = 0;
	int y = 0;
	// ячеек в инвентаре
	int width = 1;
	int height = 1;
	// стек 
	int count = 1;
	int maxStack = 1;

	// как часто встречается
	ItemRarityType rarity = ItemRarityType::Common;

	// если это экипировка, то какая
	std::optional<ItemEquipmentType> equipmentType;

	std::optional<ItemRecipe> recipe;

	QByteArray toMimeData();

	static Item fromMimeData(const QByteArray& data);

	bool compare(const Item& other);

	bool canMergeWith(const Item& other);

	void duplicate(Item& item, bool newId = true);

	bool canAcceptEquipment(const ItemEquipmentSlotType& slotType);
};
