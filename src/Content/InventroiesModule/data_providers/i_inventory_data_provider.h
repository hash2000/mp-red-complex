#pragma once
#include <QUuid>
#include <optional>

class Inventory;

class IInventoryDataProvider {
public:
	virtual ~IInventoryDataProvider() = default;

	virtual std::optional<Inventory> loadInventory(const QUuid& id) const = 0;
	virtual bool saveInventory(const QUuid& id, const Inventory& inventory) const = 0;
};
