#include "ApplicationLayer/equipment/equipments_service.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include "DataLayer/equipment/equipment.h"
#include "DataLayer/equipment/equipment_data_provider.h"
#include <QDebug>

class EquipmentsService::Private {
public:
	Private(EquipmentsService* parent)
		: q(parent) { 
	}

	EquipmentsService* q;
	EquipmentDataProvider* dataProvider;
	ItemsService* itemsService;
	std::map<QUuid, std::unique_ptr<EquipmentService>> equipments;
};

EquipmentsService::EquipmentsService(EquipmentDataProvider* dataProvider, ItemsService* itemsService, QObject* parent)
: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
	d->itemsService = itemsService;
}

EquipmentsService::~EquipmentsService() = default;

EquipmentService* EquipmentsService::equipment(const QUuid& id, bool loadIfNotExists) {
	if (!d->equipments.contains(id)) {

		if (!loadIfNotExists) {
			return nullptr;
		}

		Equipment equipmentData;
		if (!d->dataProvider->loadEquipment(id, equipmentData)) {
			return nullptr;
		}

		auto service = std::make_unique<EquipmentService>(d->itemsService);
		if (!service->load(equipmentData)) {
			return nullptr;
		}

		d->equipments.emplace(id, std::move(service));
		qDebug() << "Load equipment" << id;
	}

	return d->equipments[id].get();
}
