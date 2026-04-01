#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>

class EntityItemTooltip : public QWidget {
	Q_OBJECT

public:
	explicit EntityItemTooltip(QWidget* parent = nullptr);

	~EntityItemTooltip() override;

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
