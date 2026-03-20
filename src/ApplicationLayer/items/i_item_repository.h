#pragma once
#include "DataLayer/items/item.h"
#include <QString>
#include <QUuid>
#include <memory>
#include <vector>

/// Репозиторий для доступа к сущностям и предметам
class IItemRepository {
public:
    virtual ~IItemRepository() = default;

    /// Найти сущность предмета по идентификатору
    virtual std::unique_ptr<ItemEntity> findEntityById(const QString& id) const = 0;

    /// Получить все идентификаторы сущностей
    virtual std::vector<QString> findAllEntityIds() const = 0;

    /// Найти предмет по идентификатору (lazy loading)
    virtual std::unique_ptr<Item> findItemById(const QUuid& id) const = 0;

    /// Сохранить предмет
    virtual void saveItem(const Item& item) = 0;
};
