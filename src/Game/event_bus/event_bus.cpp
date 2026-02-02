#include "Game/event_bus/event_bus.h"

EventBus::EventBus(QObject* parent)
	: QObject(parent) {
}

EventBus::~EventBus() = default;
