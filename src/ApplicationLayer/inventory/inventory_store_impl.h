#pragma once
#include "ApplicationLayer/items_placement_store.h"
#include <QObject>
#include <memory>
#include <optional>

class ItemsService;
class InventoryDataProvider;
class ItemMimeData;

class InventoryStoreImpl : public ItemPlacementStore {
public:
	InventoryStoreImpl(InventoryDataProvider* dataProvider, ItemsService* itemsService, QObject* parent = nullptr);
	~InventoryStoreImpl() override;

	ItemPlacementService* load(const QUuid& id, bool loadIfNotExists) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
