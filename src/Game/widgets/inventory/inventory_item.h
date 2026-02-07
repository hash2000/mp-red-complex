#pragma once
#include "Game/widgets/equipment/equipment_item.h"
#include <QString>
#include <QPixmap>
#include <QJsonObject>
#include <QJsonArray>
#include <optional>
#include <vector>

// Типы ресурсов (сырьё)
enum class ResourceType {
	Sand, Soil, Clay, Stone, Granite, Basalt, Limestone,
	Coal, IronOre, GoldVein, OilShale, Aquifer, GasPocket,
	DiamondPipe, TitaniumDeposit,
};

// Типы предметов инвентаря
enum class InventoryItemType {
	Equipment, // Экипировка (оружие, броня)
	Resource, // Сырьё (ресурсы для крафта)
	Component, // Запчасти (сложные элементы из сырья)
	Container, // Ящики (могут содержать другие предметы)
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
	int rarity = 1;

	// Для экипировки — тип слота
	std::optional<Item::Type> equipmentType;

	// Для ящиков — вместимость (размер внутренней сетки)
	struct ContainerCapacity {
		int rows = 4;
		int cols = 4;
	};

	std::optional<ContainerCapacity> containerCapacity;

	// Для крафта — рецепты (опционально)
	struct Recipe {
		struct Ingredient {
			QString itemId;
			int amount;
		};
		std::vector<Ingredient> ingredients;
	};

	std::optional<Recipe> recipe;

	// Загрузка из JSON
	static InventoryItem fromJson(const QJsonObject& json);

	// Проверка, может ли предмет занимать указанную позицию в сетке
	bool canPlaceAt(int gridWidth, int x, int y) const;

	// Сериализация для drag-and-drop
	QByteArray toMimeData() const;
	static std::optional<InventoryItem> fromMimeData(const QByteArray& data);
};
