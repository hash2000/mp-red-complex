#pragma once
#include "DataLayer/items/item.h"
#include <QString>
#include <list>

struct ItemsDataProvider {
	virtual ~ItemsDataProvider() = default;

	virtual bool loadEntitiesIds(std::list<QString>& list) const = 0;
	virtual bool loadEntity(const QString& id, ItemEntity& entity) const = 0;
	virtual bool loadItem(const QString& id, Item& item) const = 0;
};
