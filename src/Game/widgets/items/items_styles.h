#pragma once
#include "DataLayer/items/item.h"
#include <QStyleOption>
#include <QProxyStyle>

class FlatPrimaryButtonStyle : public QProxyStyle {
public:
	using QProxyStyle::QProxyStyle;

	void drawControl(ControlElement element, const QStyleOption* option,
		QPainter* painter, const QWidget* widget) const override {
		if (element == CE_PushButtonLabel) {
			QStyleOptionButton myOpt(*qstyleoption_cast<const QStyleOptionButton*>(option));
			myOpt.palette.setColor(QPalette::ButtonText, QColor("#0f172a"));
			QProxyStyle::drawControl(element, &myOpt, painter, widget);
			return;
		}
		QProxyStyle::drawControl(element, option, painter, widget);
	}
};

namespace ItemsStyles {
	static constexpr int CELL_SIZE = 38;

	QString rarityColor(ItemRarityType rarity);

	// Цвета рамки по редкости
	QString borderColor(ItemRarityType rarity);

	// Градиент фона по редкости
	QString backgroundColor(ItemRarityType rarity);
}
