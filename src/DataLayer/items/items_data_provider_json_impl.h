#pragma once
#include "DataLayer/items/items_data_provider.h"
#include <QPixmap>
#include <QUuid>
#include <memory>

class Resources;

class ItemsDataProviderJsonImpl : public ItemsDataProvider {
public:
	ItemsDataProviderJsonImpl(Resources* resources);
	~ItemsDataProviderJsonImpl() override;

	bool loadEntitiesIds(std::list<QString>& list) const override;
	bool loadEntity(const QString& id, ItemEntity& entity) const override;
	bool loadItem(const QUuid& id, Item& item) const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
