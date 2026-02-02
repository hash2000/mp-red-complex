#pragma once
#include "Game/event_bus/event.h"

struct TimeEvent : public Event {
	qint64 elapsedMs; // Время с запуска приложения (мс)
	QDateTime timestamp; // Абсолютное время

	explicit TimeEvent(const QString& type, qint64 elapsed)
		: Event(type)
		, elapsedMs(elapsed)
		, timestamp(QDateTime::currentDateTime()) {
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

struct MinuteEvent : public TimeEvent {
	int minuteCount;  // Счётчик минут с запуска

	MinuteEvent(qint64 elapsed, int count)
		: TimeEvent("Minute", elapsed)
		, minuteCount(count) {
	}
};

struct SecondEvent : public TimeEvent {
	int secondCount;  // Счётчик секунд с запуска

	SecondEvent(qint64 elapsed, int count)
		: TimeEvent("Second", elapsed)
		, secondCount(count) {
	}
};

Q_DECLARE_METATYPE(TimeEvent)
Q_DECLARE_METATYPE(TickEvent)
Q_DECLARE_METATYPE(TimerEvent)
Q_DECLARE_METATYPE(MinuteEvent)
Q_DECLARE_METATYPE(SecondEvent)
