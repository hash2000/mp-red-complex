#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>

class EntityEntryWidget : public QWidget {
	Q_OBJECT

public:
	explicit EntityEntryWidget(const ItemEntity& entity, QWidget* parent = nullptr);
	~EntityEntryWidget() override;

signals:
	void createRequested(const QString& itemId);

private:
	void paintEvent(QPaintEvent* event) override;
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;
	bool eventFilter(QObject* watched, QEvent* event) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
