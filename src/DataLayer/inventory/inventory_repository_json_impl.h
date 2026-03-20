#pragma once
#include "ApplicationLayer/inventory/i_inventory_repository.h"
#include <memory>

class IInventoryDataProvider;
class IInventoryDataWriter;

/// Реализация репозитория инвентарей на основе JSON provider
class InventoryRepositoryJsonImpl : public IInventoryRepository {
public:
    InventoryRepositoryJsonImpl(
        IInventoryDataProvider* provider,
        IInventoryDataWriter* writer);
    ~InventoryRepositoryJsonImpl() override;

    std::unique_ptr<Inventory> findById(const QUuid& id) const override;
    void save(const QUuid& id, const Inventory& inventory) override;

private:
    IInventoryDataProvider* _provider;
    IInventoryDataWriter* _writer;
};
