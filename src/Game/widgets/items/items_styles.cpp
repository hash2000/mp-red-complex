#include "Game/widgets/items/items_styles.h"

namespace ItemsStyles {
	QString rarityColor(ItemRarityType rarity) {
		switch (rarity) {
		case ItemRarityType::Common:    return "#9ca3af";    // Серый
		case ItemRarityType::Improved:  return "#34d399";    // Бирюзовый
		case ItemRarityType::Stable:    return "#60a5fa";    // Голубой
		case ItemRarityType::Rare:      return "#a78bfa";    // Лавандовый
		case ItemRarityType::Unique:    return "#fbbf24";    // Янтарный
		}
		return "#ffffff";
	}

	// Цвета рамки по редкости
	QString borderColor(ItemRarityType rarity) {
		switch (rarity) {
		case ItemRarityType::Common:    return "#4a5568";
		case ItemRarityType::Improved:  return "#0ea5e9";
		case ItemRarityType::Stable:    return "#38bdf8";
		case ItemRarityType::Rare:      return "#a78bfa";
		case ItemRarityType::Unique:    return "#f59e0b";
		}
		return "#64748b";
	}

	// Градиент фона по редкости
	QString backgroundColor(ItemRarityType rarity) {
		switch (rarity) {
		case ItemRarityType::Common:    return "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1e293b, stop:1 #0f172a)";
		case ItemRarityType::Improved:  return "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0c4a6e, stop:1 #075985)";
		case ItemRarityType::Stable:    return "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #0e7490, stop:1 #155e75)";
		case ItemRarityType::Rare:      return "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4c1d95, stop:1 #3b0764)";
		case ItemRarityType::Unique:    return "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #92400e, stop:1 #78350f)";
		}
		return "qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #1e293b, stop:1 #0f172a)";
	}
}
