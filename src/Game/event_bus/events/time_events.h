#pragma once
#include "Game/event_bus/event.h"
#include <QObject>

struct TimeEvent : public Event {
	qint64 elapsedMs; // Время с запуска приложения (мс)

	explicit TimeEvent(const QString& type, qint64 elapsed)
		: Event(type)
		, elapsedMs(elapsed) {
	}
};

struct TickEvent : public TimeEvent {
	double deltaTime; // Время с предыдущего тика (секунды)

	TickEvent(qint64 elapsed, double dt)
		: TimeEvent("Tick", elapsed)
		, deltaTime(dt) {
	}
};

struct TimerEvent : public TimeEvent {
	QString timerId; // Идентификатор таймера
	int triggerCount; // Сколько раз сработал

	TimerEvent(const QString& id, qint64 elapsed, int count)
		: TimeEvent("Timer", elapsed)
		, timerId(id)
		, triggerCount(count) {
	}
};

class TimeEventBus : public QObject {
	Q_OBJECT
public:
	explicit TimeEventBus(QObject* parent = nullptr);
	~TimeEventBus() override;

	void postTick(const TickEvent& event);
	void postTimer(const TimerEvent& event);
	void postPaused();
	void postResumed();

signals:
	void tick(const TickEvent& event);
	void timer(const TimerEvent& event);
	void paused();
	void resumed();
};
