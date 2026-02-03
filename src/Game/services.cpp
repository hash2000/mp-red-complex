#include "Game/services.h"
#include "Game/event_bus/event_bus.h"
#include "Game/services/time_service.h"
#include "Game/services/world_service.h"

class Services::Private {
public:
	Private(Services* parent)
		: q(parent) {
	}

	Services* q;
	EventBus* eventBus;
	std::shared_ptr<TimeService> timeService;
	std::shared_ptr<WorldService> worldService;
};


Services::Services(EventBus* eventBus)
: d(std::make_unique<Private>(this)) {
	d->timeService = std::make_unique<TimeService>(eventBus);
	d->worldService = std::make_unique<WorldService>(eventBus);
	d->eventBus = eventBus;
}

Services::~Services() = default;

void Services::run() {
	d->timeService->start();
}

TimeService* Services::timeService() const {
	return d->timeService.get();
}

WorldService* Services::worldService() const {
	return d->worldService.get();
}
