#pragma once
#include "DataLayer/equipment/equipment_item.h"
#include <QString>
#include <QPixmap>
#include <QList>
#include <optional>
#include <QJsonObject>
#include <QJsonArray>
#include <memory>
#include <map>

enum class InventoryResourceType {
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

enum class InventoryItemType {
	Equipment, // Экипировка (оружие, броня)
	Resource, // Сырьё (ресурсы для крафта)
	Component, // Запчасти (сложные элементы из сырья)
	Container, // Ящики (могут содержать другие предметы)
};

// Для ящиков — вместимость (размер внутренней сетки)
struct InventoryItemContainerCapacity {
	int rows = 4;
	int cols = 4;
};

// рецепты
struct InventoryItemRecipe {
	struct Ingredient {
		QString itemId;
		int amount;
	};
	std::vector<Ingredient> ingredients;
};

struct InventoryHandler {
	QString id;
	QString name;
	InventoryItemType type;
	int x = 0;
	int y = 0;

	// Размер в ячейках сетки (1x1 по умолчанию)
	int width = 1;
	int height = 1;

	int count = 1;
	int maxStack = 1;

	QPixmap icon;

	QByteArray toMimeData() const;

	static InventoryHandler fromMimeData(const QByteArray& data);
};

struct InventoryItem : public InventoryHandler {
	QString entityId;
	QString description;
	QString iconPath;

	EquipmentItemRarityType rarity = EquipmentItemRarityType::Common;

	std::optional<EquipmentItemType> equipmentType;

	std::optional<InventoryItemContainerCapacity> containerCapacity;

	std::optional<InventoryItemRecipe> recipe;

	static std::shared_ptr<InventoryItem> fromJson(const QJsonObject& json);

	bool compare(const InventoryHandler& item);
	bool canMergeWith(const InventoryItem& other) const;

	std::shared_ptr<InventoryItem> duplicate(bool newId = true) const;
};

struct Inventory {
	QString name;
	QString id;
	std::map<QString, std::shared_ptr<InventoryItem>> items;
	int rows;
	int cols;
};
