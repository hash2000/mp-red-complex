#pragma once
#include "ApplicationLayer/repositories/i_inventory_repository.h"
#include <memory>

class InventoryDataProvider;
class InventoryDataWriter;

/// Реализация репозитория инвентарей на основе JSON provider
class InventoryRepositoryJsonImpl : public IInventoryRepository {
public:
    InventoryRepositoryJsonImpl(
        InventoryDataProvider* provider,
        InventoryDataWriter* writer);
    ~InventoryRepositoryJsonImpl() override;

    std::unique_ptr<Inventory> findById(const QUuid& id) const override;
    void save(const QUuid& id, const Inventory& inventory) override;

private:
    InventoryDataProvider* _provider;
    InventoryDataWriter* _writer;
};
