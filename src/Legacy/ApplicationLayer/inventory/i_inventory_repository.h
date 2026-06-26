#pragma once
#include <QUuid>
#include <memory>

class Inventory;

/// Репозиторий для доступа к инвентарям
class IInventoryRepository {
public:
    virtual ~IInventoryRepository() = default;

    /// Загрузить инвентарь по идентификатору
    virtual std::unique_ptr<Inventory> findById(const QUuid& id) const = 0;

    /// Сохранить инвентарь
    virtual void save(const QUuid& id, const Inventory& inventory) = 0;
};
