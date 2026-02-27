#include "ApplicationLayer/equipment/equipment_service.h"
#include "ApplicationLayer/equipment/equipment_item_handler.h"

class EquipmentService::Private {
public:
	Private(EquipmentService* parent)
		: q(parent) {
	}

	EquipmentService* q;
	EquipmentDataProvider* dataProvider;
	ItemsService* itemsService;
	std::map<QString, std::unique_ptr<EquipmentItemHandler>> inventoryItems;
};

EquipmentService::EquipmentService(EquipmentDataProvider* dataProvider, ItemsService* itemsService, QObject* parent)
	: d(std::make_unique<Private>(this)) {
	d->dataProvider = dataProvider;
	d->itemsService = itemsService;
}

EquipmentService::~EquipmentService() = default;

bool EquipmentService::load(const QUuid& id, bool loadIfNotExists) {


	return true;
}
