#pragma once
#include "ApplicationLayer/items/i_item_repository.h"
#include <memory>

class IItemsDataProvider;

/// Реализация репозитория предметов на основе JSON provider
class ItemRepositoryJsonImpl : public IItemRepository {
public:
    explicit ItemRepositoryJsonImpl(IItemsDataProvider* provider);
    ~ItemRepositoryJsonImpl() override;

    std::unique_ptr<ItemEntity> findEntityById(const QString& id) const override;
    std::vector<QString> findAllEntityIds() const override;
    std::unique_ptr<Item> findItemById(const QUuid& id) const override;
    void saveItem(const Item& item) override;

private:
    IItemsDataProvider* _provider;
};
