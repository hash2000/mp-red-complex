#include "Game/event_bus/event_bus.h"
#include "Game/event_bus/events/time_events.h"

class EventBus::Private {
public:
  Private(EventBus* parent)
	: q(parent) {
  }

  EventBus* q;
	std::unique_ptr<TimeEventBus> timeEventBus;
};


EventBus::EventBus()
  : d(std::make_unique<Private>(this)) {
	d->timeEventBus = std::make_unique<TimeEventBus>();
}

EventBus::~EventBus() = default;

TimeEventBus* EventBus::timeEventBus() const {
	return d->timeEventBus.get();
}
