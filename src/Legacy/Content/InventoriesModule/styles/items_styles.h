#pragma once
#include <QStyleOption>
#include <QProxyStyle>

enum class ItemRarityType;

class FlatPrimaryButtonStyle : public QProxyStyle {
public:
	using QProxyStyle::QProxyStyle;

	void drawControl(ControlElement element,
		const QStyleOption* option,
		QPainter* painter,
		const QWidget* widget) const override;
};

namespace ItemsStyles {
	static constexpr int CELL_SIZE = 38;
	static constexpr int ICON_SIZE = 26;

	QString rarityColor(ItemRarityType rarity);

	// Цвета рамки по редкости
	QString borderColor(ItemRarityType rarity);

	// Градиент фона по редкости
	QString backgroundColor(ItemRarityType rarity);
}
