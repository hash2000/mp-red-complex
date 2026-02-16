#pragma once
#include "DataLayer/items/item.h"
#include <QString>

struct ItemsDataProvider {
	virtual ~ItemsDataProvider() = default;

	virtual bool load(const QString& id, Item& item) const = 0;
};
