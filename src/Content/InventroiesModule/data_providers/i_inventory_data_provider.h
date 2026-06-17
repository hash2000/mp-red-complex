#pragma once
#include <QUuid>
#include <optional>

class Inventory;

class IInventoryDataProvider {
public:
	virtual ~IInventoryDataProvider() = default;

	virtual std::shared_ptr<Inventory> inventory(const QUuid& id) const = 0;
	virtual bool save(const QUuid& id, const Inventory& inventory) const = 0;
};
