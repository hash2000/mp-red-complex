#pragma once
#include "ApplicationLayer/equipment/i_equipment_repository.h"
#include <memory>

class IEquipmentDataProvider;
class IEquipmentDataWriter;

/// Реализация репозитория экипировок на основе JSON provider
class EquipmentRepositoryJsonImpl : public IEquipmentRepository {
public:
    EquipmentRepositoryJsonImpl(
        IEquipmentDataProvider* provider,
        IEquipmentDataWriter* writer);
    ~EquipmentRepositoryJsonImpl() override;

    std::unique_ptr<Equipment> findById(const QUuid& id) const override;
    void save(const QUuid& id, const Equipment& equipment) override;

private:
    IEquipmentDataProvider* _provider;
    IEquipmentDataWriter* _writer;
};
