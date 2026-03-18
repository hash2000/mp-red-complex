#include "DataLayer/equipment/equipment_repository_json_impl.h"
#include "DataLayer/equipment/equipment_data_provider.h"
#include "DataLayer/equipment/equipment_data_writer.h"
#include "DataLayer/equipment/equipment.h"
#include <QDebug>

EquipmentRepositoryJsonImpl::EquipmentRepositoryJsonImpl(
    EquipmentDataProvider* provider,
    EquipmentDataWriter* writer)
    : _provider(provider)
    , _writer(writer) {
}

EquipmentRepositoryJsonImpl::~EquipmentRepositoryJsonImpl() = default;

std::unique_ptr<Equipment> EquipmentRepositoryJsonImpl::findById(const QUuid& id) const {
    auto equipment = std::make_unique<Equipment>();
    if (!_provider->loadEquipment(id, *equipment)) {
        return nullptr;
    }
    return equipment;
}

void EquipmentRepositoryJsonImpl::save(const QUuid& id, const Equipment& equipment) {
	if (!_writer->saveEquipment(id, equipment)) {
        qWarning() << "EquipmentRepositoryJsonImpl::save: failed to save equipment" << id;
    }
}
