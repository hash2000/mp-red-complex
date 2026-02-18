#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>

class ItemEntityEntryWidget : public QWidget {
	Q_OBJECT

public:
	explicit ItemEntityEntryWidget(const ItemEntity& entity, QWidget* parent = nullptr);
	~ItemEntityEntryWidget() override;

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
