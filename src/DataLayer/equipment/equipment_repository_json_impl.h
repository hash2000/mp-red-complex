#pragma once
#include "ApplicationLayer/equipment/i_equipment_repository.h"
#include <memory>

class EquipmentDataProvider;
class EquipmentDataWriter;

/// Реализация репозитория экипировок на основе JSON provider
class EquipmentRepositoryJsonImpl : public IEquipmentRepository {
public:
    EquipmentRepositoryJsonImpl(
        EquipmentDataProvider* provider,
        EquipmentDataWriter* writer);
    ~EquipmentRepositoryJsonImpl() override;

    std::unique_ptr<Equipment> findById(const QUuid& id) const override;
    void save(const QUuid& id, const Equipment& equipment) override;

private:
    EquipmentDataProvider* _provider;
    EquipmentDataWriter* _writer;
};
