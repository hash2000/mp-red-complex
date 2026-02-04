#include "Game/services.h"
#include "Game/services/time_service/time_service.h"
#include "Game/services/world_service/world_service.h"

class Services::Private {
public:
	Private(Services* parent)
		: q(parent) {
	}

	Services* q;
	std::shared_ptr<TimeService> timeService;
	std::shared_ptr<WorldService> worldService;
};


Services::Services()
: d(std::make_unique<Private>(this)) {
	d->timeService = std::make_unique<TimeService>();
	d->worldService = std::make_unique<WorldService>();
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
