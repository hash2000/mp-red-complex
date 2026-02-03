#pragma once
#include <QObject>
#include <QVariant>
#include <QMetaType>
#include <typeinfo>
#include <QDebug>
#include <memory>

class TimeEventBus;

class EventBus {
public:
	explicit EventBus();
	~EventBus();

	TimeEventBus* timeEventBus() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
