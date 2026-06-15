#pragma once
#include "Content/InventroiesModule/models/item.h"
#include <QString>
#include <list>
#include <memory>

class IItemsDataProvider {
public:
	virtual ~IItemsDataProvider() = default;

	virtual std::list<std::unique_ptr<ItemEntity>> entities() const = 0;
	virtual std::unique_ptr<ItemEntity> entity(const QString& id) const = 0;
	virtual std::unique_ptr<Item> item(const QUuid& id) const = 0;
	virtual bool setItem(const QUuid& id, const Item& item) const = 0;
};
