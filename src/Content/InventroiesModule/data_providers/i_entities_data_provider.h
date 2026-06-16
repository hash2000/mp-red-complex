#pragma once
#include "Content/InventroiesModule/models/item.h"
#include <QString>
#include <list>
#include <memory>

class IEntitiesDataProvider {
public:
	virtual ~IEntitiesDataProvider() = default;

	virtual std::list<std::shared_ptr<ItemEntity>> entities() const = 0;
	virtual std::shared_ptr<ItemEntity> entity(const QString& id) const = 0;
};
