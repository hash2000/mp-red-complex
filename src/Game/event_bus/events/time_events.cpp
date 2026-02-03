#include "Game/event_bus/events/time_events.h"

TimeEventBus::TimeEventBus(QObject* parent)
	: QObject(parent) {
}

TimeEventBus::~TimeEventBus() = default;

void TimeEventBus::postTick(const TickEvent& event) {
	emit tick(event);
}

void TimeEventBus::postTimer(const TimerEvent& event) {
	emit timer(event);
}

void TimeEventBus::postPaused() {
	emit paused();
}

void TimeEventBus::postResumed() {
	emit resumed();
}

