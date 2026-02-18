#pragma once
#include "DataLayer/items/item.h"
#include <QString>
#include <list>

struct ItemsDataProvider {
	virtual ~ItemsDataProvider() = default;

	virtual bool loadEntitiesIds(std::list<QString>& list) = 0;
	virtual bool loadEntity(const QString& id, ItemEntity& entity) const = 0;
};
