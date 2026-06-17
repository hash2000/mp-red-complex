#pragma once
#include <QUuid>
#include <list>

class IInventoriesDataProvider {
public:
	virtual ~IInventoriesDataProvider() = default;

	virtual void inventories(std::list<QUuid>& inventories) const = 0;
	virtual void equipments(std::list<QUuid>& equipments) const = 0;
};

