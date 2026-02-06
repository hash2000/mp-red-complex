#include "Game/services/inventory_service/inventory_service.h"

class InventoryService::Private {
public:
	Private(InventoryService* parent)
	: q(parent) {
	}

	InventoryService* q;
};

InventoryService::InventoryService(QObject* parent)
: d(std::make_unique<Private>(this)) {
}

InventoryService::~InventoryService() = default;

void InventoryService::onSave() {

}

void InventoryService::onLoad() {

}
