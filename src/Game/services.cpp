#include "Game/services.h"
#include "Game/services/time_service.h"
#include "Game/event_bus/event_bus.h"

class Services::Private {
public:
	Private(Services* parent)
		: q(parent) {
	}

	Services* q;
	EventBus* eventBus;
	std::shared_ptr<TimeService> timeService;
};


Services::Services(EventBus* eventBus)
: d(new Private(this)) {
	d->timeService = std::unique_ptr<TimeService>();
	d->eventBus = eventBus;
}

Services::~Services() = default;

TimeService* Services::timeService() const {
	return d->timeService.get();
}
