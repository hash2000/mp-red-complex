#include "ApplicationLayer/equipment/equipment_store_impl.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include "DataLayer/equipment/equipment_data_provider.h"
#include "DataLayer/equipment/equipment.h"
#include <QDebug>
#include <map>

class EquipmentStoreImpl::Private {
public:
	Private(EquipmentStoreImpl* parent)
		: q(parent) {
	}

	EquipmentStoreImpl* q;
	EquipmentDataProvider* dataProvider;
	ItemsService* itemsService;
	std::map<QUuid, std::unique_ptr<ItemPlacementService>> services;
};

EquipmentStoreImpl::EquipmentStoreImpl(EquipmentDataProvider* dataProvider, ItemsService* itemsService, QObject* parent)
	: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
	d->itemsService = itemsService;
}

EquipmentStoreImpl::~EquipmentStoreImpl() = default;

ItemPlacementService* EquipmentStoreImpl::load(const QUuid& id, bool loadIfNotExists) {
	const auto& it = d->services.find(id);
	if (it != d->services.end()) {
		return it->second.get();
	}

	if (!loadIfNotExists) {
		return nullptr;
	}

	Equipment equipmentData;
	if (!d->dataProvider->loadEquipment(id, equipmentData)) {
		return nullptr;
	}

	auto equipment = std::make_unique<EquipmentService>(d->itemsService);
	if (!equipment->load(equipmentData)) {
		return nullptr;
	}

	qDebug() << "Load equipment" << id;

	const auto &insertREsult = d->services.emplace(id, std::move(equipment));

	return insertREsult.first->second.get();
}
