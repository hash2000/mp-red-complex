#pragma once
#include <QString>
#include <QColor>
#include <vector>
#include <random>

enum class BiomeType {
	Empty,
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
	TitaniumDeposit
};

struct ResourceElement {
	QString _name;
	float _percentage = 0.0f; // 0.0–100.0
};

struct Tile {
	BiomeType _biome = BiomeType::Empty;
	float _destruction = 0.0f; // 0–100%
	bool isWalkable() const {
		return _destruction >= 100.0f;
	}

	std::vector<ResourceElement> _resources; // до 3 элементов

	QColor color() const {
		if (isWalkable()) return QColor("#222222"); // чёрный — пустота
		switch (_biome) {
		case BiomeType::Sand: return QColor("#E6C98C");
		case BiomeType::Soil: return QColor("#8B5A2B");
		case BiomeType::Clay: return QColor("#C97A6D");
		case BiomeType::Stone: return QColor("#808080");
		case BiomeType::Granite: return QColor("#A09090");
		case BiomeType::Basalt: return QColor("#333333");
		case BiomeType::Limestone: return QColor("#F0E6D2");
		case BiomeType::Coal: return QColor("#1A1A1A");
		case BiomeType::IronOre: return QColor("#B7410E");
		case BiomeType::GoldVein: return QColor("#FFD700");
		case BiomeType::OilShale: return QColor("#2F2F2F");
		case BiomeType::Aquifer: return QColor("#87CEEB");
		case BiomeType::GasPocket: return QColor("#ADD8E6");
		case BiomeType::DiamondPipe: return QColor("#B9F2FF");
		case BiomeType::TitaniumDeposit: return QColor("#4A90A4");
		default:
			return QColor("#000000");
		}
	}

	static QString biomeName(BiomeType b) {
		switch (b) {
		case BiomeType::Empty: return "Empty";
		case BiomeType::Sand: return "Sand";
		case BiomeType::Soil: return "Soil";
		case BiomeType::Clay: return "Clay";
		case BiomeType::Stone: return "Stone";
		case BiomeType::Granite: return "Granite";
		case BiomeType::Basalt: return "Basalt";
		case BiomeType::Limestone: return "Limestone";
		case BiomeType::Coal: return "Coal";
		case BiomeType::IronOre: return "Iron Ore";
		case BiomeType::GoldVein: return "Gold Vein";
		case BiomeType::OilShale: return "Oil Shale";
		case BiomeType::Aquifer: return "Aquifer";
		case BiomeType::GasPocket: return "Gas Pocket";
		case BiomeType::DiamondPipe: return "Diamond Pipe";
		case BiomeType::TitaniumDeposit: return "Titanium Deposit";
		default:
			return "Unknown";
		}
	}
};
