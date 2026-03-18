#pragma once
#include <QUuid>
#include <memory>

class Equipment;

/// Репозиторий для доступа к экипировкам
class IEquipmentRepository {
public:
    virtual ~IEquipmentRepository() = default;

    /// Загрузить экипировку по идентификатору
    virtual std::unique_ptr<Equipment> findById(const QUuid& id) const = 0;

    /// Сохранить экипировку
    virtual void save(const QUuid& id, const Equipment& equipment) = 0;
};
