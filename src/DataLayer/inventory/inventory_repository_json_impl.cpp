#include "DataLayer/inventory/inventory_repository_json_impl.h"
#include "DataLayer/inventory/inventory_data_provider.h"
#include "DataLayer/inventory/inventory_data_writer.h"
#include "DataLayer/inventory/inventory_item.h"
#include <QDebug>

InventoryRepositoryJsonImpl::InventoryRepositoryJsonImpl(
    InventoryDataProvider* provider,
    InventoryDataWriter* writer)
    : _provider(provider)
    , _writer(writer) {
}

InventoryRepositoryJsonImpl::~InventoryRepositoryJsonImpl() = default;

std::unique_ptr<Inventory> InventoryRepositoryJsonImpl::findById(const QUuid& id) const {
    auto inventory = std::make_unique<Inventory>();
    if (!_provider->loadInventory(id, *inventory)) {
        return nullptr;
    }
    return inventory;
}

void InventoryRepositoryJsonImpl::save(const QUuid& id, const Inventory& inventory) {
    if (!_writer->saveInventory(id, inventory)) {
        qWarning() << "InventoryRepositoryJsonImpl::save: failed to save inventory" << id;
    }
}
