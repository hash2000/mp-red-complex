#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>
#include <memory>

class ItemsService;

class ItemsWidget : public QWidget {
	Q_OBJECT
public:
	explicit ItemsWidget(ItemsService* service, QWidget* parent = nullptr);
	~ItemsWidget() override;

	void addItemEntity(const ItemEntity& entity);

	void clear();

signals:
	// Сигнал, который вы сможете подключить к своему обработчику
	void itemCreateRequested(const QString& itemId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
