#pragma once
#include "ApplicationLayer/inventory_loader.h"
#include <QObject>
#include <memory>
#include <optional>

class InventoryService;
class ItemsService;
class InventoryDataProvider;
class ItemMimeData;

class InventoryLoaderImpl : public InventoryLoader {
public:
	InventoryLoaderImpl(InventoryDataProvider* dataProvider, ItemsService* itemsService, QObject* parent = nullptr);
	~InventoryLoaderImpl() override;

	std::unique_ptr<ItemPlacement> load(const QUuid& id) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
