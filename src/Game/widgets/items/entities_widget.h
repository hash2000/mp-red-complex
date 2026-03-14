#pragma once
#include "DataLayer/items/item.h"
#include <QWidget>
#include <memory>

class ItemsService;

class EntitiesWidget : public QWidget {
	Q_OBJECT
public:
	explicit EntitiesWidget(ItemsService* service, QWidget* parent = nullptr);
	~EntitiesWidget() override;

	void addItemEntity(const ItemEntity& entity);

	void clear();

signals:
	void itemCreateRequested(const QString& itemId);

private:
	class Private;
	std::unique_ptr<Private> d;
};
