#pragma once
#include "Content/InventroiesModule/models/item.h"
#include <QString>
#include <memory>
#include <list>

class IItemsDataProvider {
public:
	virtual ~IItemsDataProvider() = default;
	virtual std::shared_ptr<Item> item(const QUuid& id) const = 0;
	virtual std::list<std::shared_ptr<Item>> containers() const = 0;
	virtual std::list<std::shared_ptr<Item>> equipments() const = 0;
	virtual std::list<std::shared_ptr<Item>> itemsFromContainer(const QUuid& constainerId) const = 0;
	virtual bool changeContainer(const Item& item, const QUuid& constainerId) const = 0;
	virtual bool save(const Item& item) const = 0;
};
