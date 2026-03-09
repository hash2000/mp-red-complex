#pragma once
#include "ApplicationLayer/items_placement_store.h"
#include <QObject>
#include <memory>
#include <optional>

class ItemsService;
class EquipmentDataProvider;

class EquipmentStoreImpl : public ItemPlacementStore {
public:
	EquipmentStoreImpl(EquipmentDataProvider* dataProvider, ItemsService* itemsService, QObject* parent = nullptr);
	~EquipmentStoreImpl() override;

	ItemPlacementService* load(const QUuid& id, bool loadIfNotExists) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
