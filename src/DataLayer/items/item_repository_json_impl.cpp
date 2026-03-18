#include "DataLayer/items/item_repository_json_impl.h"
#include "DataLayer/items/items_data_provider.h"
#include <QDebug>

ItemRepositoryJsonImpl::ItemRepositoryJsonImpl(ItemsDataProvider* provider)
    : _provider(provider) {
}

ItemRepositoryJsonImpl::~ItemRepositoryJsonImpl() = default;

std::unique_ptr<ItemEntity> ItemRepositoryJsonImpl::findEntityById(const QString& id) const {
    auto entity = std::make_unique<ItemEntity>();
    if (!_provider->loadEntity(id, *entity)) {
        qWarning() << "ItemRepositoryJsonImpl::findEntityById: failed to load entity" << id;
        return nullptr;
    }
    return entity;
}

std::vector<QString> ItemRepositoryJsonImpl::findAllEntityIds() const {
    std::vector<QString> result;
    std::list<QString> ids;
    if (_provider->loadEntitiesIds(ids)) {
        result.assign(ids.begin(), ids.end());
    }
    return result;
}

std::unique_ptr<Item> ItemRepositoryJsonImpl::findItemById(const QUuid& id) const {
    auto item = std::make_unique<Item>();
    if (!_provider->loadItem(id, *item)) {
        return nullptr;
    }
    return item;
}

void ItemRepositoryJsonImpl::saveItem(const Item& item) {
    // TODO: реализовать сохранение через ItemsDataWriter
    Q_UNUSED(item);
}
