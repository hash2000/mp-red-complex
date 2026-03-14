#pragma once
#include <QUuid>
#include <memory>

class ItemPlacementService;

class ItemPlacementStore {
public:
	virtual ~ItemPlacementStore() = default;

	virtual ItemPlacementService* load(const QUuid& id, bool loadIfNotExists) = 0;
};
