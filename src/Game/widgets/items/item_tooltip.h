#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>

class ItemTooltip : public QWidget {
	Q_OBJECT

public:
	explicit ItemTooltip(QWidget* parent = nullptr);

	~ItemTooltip() override;

	void showForItem(const ItemEntity& item, const QPoint& globalPos);
	void hideImmediately();
	QString currentItem() const;

private:
	void timerEvent(QTimerEvent* event) override;
	void showAnimated();

private:
	class Private;
	std::unique_ptr<Private> d;
};
