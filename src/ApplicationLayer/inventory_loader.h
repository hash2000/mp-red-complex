#pragma once
#include <QUuid>
#include <memory>

class ItemPlacement;

class InventoryLoader {
public:
	virtual ~InventoryLoader() = default;

	virtual std::unique_ptr<ItemPlacement> load(const QUuid& id) = 0;
};
