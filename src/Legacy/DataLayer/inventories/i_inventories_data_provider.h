#pragma once
#include <QUuid>
#include <list>

class IInventoriesDataProvider {
public:
	virtual ~IInventoriesDataProvider() = default;

	virtual void loadInventories(std::list<QUuid>& inventories) const = 0;
	virtual void loadEquipments(std::list<QUuid>& equipments) const = 0;
};

