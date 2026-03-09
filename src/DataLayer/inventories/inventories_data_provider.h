#pragma once
#include <QUuid>
#include <list>

class InventoriesDataProvider {
public:
	virtual ~InventoriesDataProvider() = default;

	virtual void loadInventories(std::list<QUuid>& inventories) = 0;
	virtual void loadEquipments(std::list<QUuid>& equipments) = 0;
};

