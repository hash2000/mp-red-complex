#include "ApplicationLayer/equipment/equipment_save_handler.h"
#include "ApplicationLayer/equipment/equipment_service.h"
#include "ApplicationLayer/equipment/equipment_item_handler.h"
#include "DataLayer/equipment/i_equipment_data_writer.h"
#include "DataLayer/equipment/equipment.h"
#include <QDebug>

class EquipmentSaveHandler::Private {
public:
	Private(EquipmentSaveHandler* parent)
		: q(parent) {
	}

	Equipment toEquipmentData(const EquipmentService* service) {
		Equipment equipment;
		equipment.id = service->placementId();

		for (const auto& [slot, item] : service->items()) {
			if (!item) {
				continue;
			}

			EquipmentItem eqItem;
			eqItem.id = item->id;
			eqItem.slot = item->slot;
			equipment.items.push_back(eqItem);
		}

		return equipment;
	}

	bool save() {
		if (!equipmentService || !dataWriter) {
			return false;
		}

		auto equipmentData = toEquipmentData(equipmentService);
		return dataWriter->saveEquipment(equipmentId, equipmentData);
	}

	EquipmentSaveHandler* q;
	EquipmentService* equipmentService = nullptr;
	IEquipmentDataWriter* dataWriter = nullptr;
	QUuid equipmentId;
};

EquipmentSaveHandler::EquipmentSaveHandler(
	EquipmentService* equipmentService,
	IEquipmentDataWriter* dataWriter,
	const QUuid& equipmentId,
	QObject* parent)
: QObject(parent)
, d(std::make_unique<Private>(this)) {
	d->equipmentService = equipmentService;
	d->dataWriter = dataWriter;
	d->equipmentId = equipmentId;

	// Подключаемся ко всем сигналам изменений
	connect(equipmentService, &EquipmentService::itemEquipped,
		this, &EquipmentSaveHandler::onItemChanged);
	connect(equipmentService, &EquipmentService::itemUnequipped,
		this, &EquipmentSaveHandler::onItemChanged);
	connect(equipmentService, &EquipmentService::removeItemEvent,
		this, &EquipmentSaveHandler::onItemChanged);
}

EquipmentSaveHandler::~EquipmentSaveHandler() = default;

void EquipmentSaveHandler::onItemChanged() {
	if (d->save()) {
		qDebug() << "Equipment saved:" << d->equipmentId;
	}
	else {
		qWarning() << "Failed to save equipment:" << d->equipmentId;
	}
}
