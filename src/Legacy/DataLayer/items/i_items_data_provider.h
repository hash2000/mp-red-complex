#pragma once
#include "DataLayer/items/item.h"
#include <QString>
#include <list>

class IItemsDataProvider {
public:
	virtual ~IItemsDataProvider() = default;

	virtual bool loadEntitiesIds(std::list<QString>& list) const = 0;
	virtual bool loadEntity(const QString& id, ItemEntity& entity) const = 0;
	virtual bool loadItem(const QUuid& id, Item& item) const = 0;
};
