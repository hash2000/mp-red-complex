#include "Game/services.h"
#include "Game/services/time_service/time_service.h"
#include "Game/services/world_service/world_service.h"
#include "Game/services/inventory_service/inventory_service.h"
#include "DataLayer/inventory/inventory_data_provider_fs_impl.h"
#include <list>

class Services::Private {
public:
	Private(Services* parent)
		: q(parent) {
	}

	Services* q;
	std::unique_ptr<TimeService> timeService;
	std::unique_ptr<WorldService> worldService;
	std::unique_ptr<InventoryService> inventoryService;
	std::unique_ptr<InventoryDataProvider> inventoryDataProvider;
};


Services::Services(Resources* resources)
: d(std::make_unique<Private>(this)) {
	d->inventoryDataProvider = std::make_unique<InventoryDataProviderFilesistemImpl>(resources);
	d->timeService = std::make_unique<TimeService>();

	d->worldService = std::make_unique<WorldService>();
	connect(this, &Services::save, d->worldService.get(), &WorldService::onSave);
	connect(this, &Services::load, d->worldService.get(), &WorldService::onLoad);

	d->inventoryService = std::make_unique<InventoryService>(d->inventoryDataProvider.get());
	connect(this, &Services::save, d->inventoryService.get(), &InventoryService::onSave);
	connect(this, &Services::load, d->inventoryService.get(), &InventoryService::onLoad);
}

Services::~Services() = default;

void Services::run() {
	d->timeService->start();
}

void Services::postLoadEvent() {
	emit load();
}

void Services::postSaveEvent() {
	emit save();
}

TimeService* Services::timeService() const {
	return d->timeService.get();
}

WorldService* Services::worldService() const {
	return d->worldService.get();
}

InventoryService* Services::inventoryService() const {
	return d->inventoryService.get();
}
