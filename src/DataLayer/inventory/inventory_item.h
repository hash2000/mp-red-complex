#pragma once
#include "DataLayer/equipment/equipment_item.h"
#include <QString>
#include <QPixmap>
#include <optional>
#include <vector>
#include <QJsonObject>
#include <QJsonArray>

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

struct InventoryItem {
	QString id;
	QString name;
	QString description;
	QPixmap icon;
	InventoryItemType type;

	// Размер в ячейках сетки (1x1 по умолчанию)
	int width = 1;
	int height = 1;

	// Для стекируемых предметов (сырьё, запчасти)
	int maxStack = 1;
	int count = 1;
	EquipmentItemRarityType rarity = EquipmentItemRarityType::Common;

	std::optional<EquipmentItemType> equipmentType;

	std::optional<InventoryItemContainerCapacity> containerCapacity;

	std::optional<InventoryItemRecipe> recipe;

	QByteArray toMimeData() const;

	static std::optional<InventoryItem> fromMimeData(const QByteArray& data);

	static InventoryItem fromJson(const QJsonObject& json);
};
