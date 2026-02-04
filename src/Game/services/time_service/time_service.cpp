#include "Game/services/time_service/time_service.h"
#include "Game/services/time_service/time_events.h"
#include <QTimer>
#include <QDateTime>
#include <QHash>
#include <QElapsedTimer>

class TimeService::Private {
public:
	Private(TimeService* parent)
	:q(parent) {
	}

	struct ScheduledEvent {
		QString timerId;          // Идентификатор таймера
		qint64 triggerTime;       // Абсолютное время срабатывания (мс с запуска)
		int intervalMs;           // 0 = однократный, >0 = повторяющийся
		int triggerCount;         // Счётчик срабатываний
	};

	TimeService* q;

	QTimer mainTimer;
	QElapsedTimer elapsedTimer;
	bool isRunning = false;
	double deltaTime = 0.0;

	// Очередь запланированных событий (сортируется по triggerTime)
	QList<ScheduledEvent> scheduledEvents;
};

TimeService::TimeService(QObject* parent)
	: d(std::make_unique<Private>(this)) {

	// Настройка основного таймера (~60 Гц)
	d->mainTimer.setInterval(16);
	d->mainTimer.setTimerType(Qt::PreciseTimer);

	connect(&d->mainTimer, &QTimer::timeout, this, &TimeService::onMainTimerTimeout);
}

TimeService::~TimeService() {
	stop();
}

void TimeService::start() {
	if (d->isRunning) {
		return;
	}

	d->elapsedTimer.start();
	d->mainTimer.start();
	d->isRunning = true;
	emit resumed();

	qInfo() << "TimeService started";
}

void TimeService::stop() {
	if (!d->isRunning) {
		return;
	}

	d->mainTimer.stop();
	d->isRunning = false;
	d->scheduledEvents.clear();
	emit paused();

	qInfo() << "TimeService stopped";
}

bool TimeService::isRunning() const {
	return d->isRunning;
}

void TimeService::onMainTimerTimeout() {
	qint64 currentTime = d->elapsedTimer.elapsed();
	static qint64 lastTime = 0;

	// Расчёт дельты с защитой от аномалий (например, сворачивание окна)
	d->deltaTime = (currentTime > lastTime)
		? (currentTime - lastTime) / 1000.0
		: 0.016; // fallback к ~60 Гц

	lastTime = currentTime;

	emit tick(TickEvent(currentTime, d->deltaTime));
	checkAndTriggerEvents(currentTime);
}

qint64 TimeService::elapsedMilliseconds() const {
	return d->elapsedTimer.elapsed();
}

QDateTime TimeService::currentTime() const {
	return QDateTime::currentDateTime();
}

// Создание однократного таймера (вызовется один раз через задержку)
void TimeService::singleShot(int ms, const QString& timerId) {
	if (ms < 0) {
		return;
	}

	// Отмена существующего таймера с таким же ID
	stopTimer(timerId);

	qint64 currentTime = d->elapsedTimer.elapsed();
	qint64 triggerTime = currentTime + ms;

	Private::ScheduledEvent event{
			timerId,
			triggerTime,
			0,  // однократный
			0
	};

	// Вставка с сохранением сортировки по времени срабатывания
	int pos = findInsertPosition(triggerTime);
	d->scheduledEvents.insert(pos, event);
}

// Создание повторяющегося таймера
void TimeService::startTimer(int intervalMs, const QString& timerId) {
	if (intervalMs <= 0) {
		return;
	}

	stopTimer(timerId);

	qint64 currentTime = d->elapsedTimer.elapsed();
	qint64 triggerTime = currentTime + intervalMs;

	Private::ScheduledEvent event{
			timerId,
			triggerTime,
			intervalMs,
			0
	};

	int pos = findInsertPosition(triggerTime);
	d->scheduledEvents.insert(pos, event);
}

bool TimeService::stopTimer(const QString& timerId) {
	for (int i = 0; i < d->scheduledEvents.size(); i++) {
		if (d->scheduledEvents[i].timerId == timerId) {
			d->scheduledEvents.removeAt(i);
			return true;
		}
	}
	return false;
}

bool TimeService::hasTimer(const QString& timerId) const {
	for (const auto& event : d->scheduledEvents) {
		if (event.timerId == timerId) {
			return true;
		}
	}
	return false;
}

void TimeService::checkAndTriggerEvents(qint64 currentTime) {
	// Обработка событий с начала очереди (самые ранние)
	while (!d->scheduledEvents.isEmpty() &&
		d->scheduledEvents.first().triggerTime <= currentTime) {

		Private::ScheduledEvent event = d->scheduledEvents.takeFirst();
		event.triggerCount++;

		emit timer(TimerEvent(event.timerId, currentTime, event.triggerCount));

		// Для повторяющихся событий — перепланирование
		if (event.intervalMs > 0) {
			qint64 nextTrigger = ((currentTime / event.intervalMs) + 1) * event.intervalMs;
			event.triggerTime = nextTrigger;

			// Вставка с сохранением сортировки
			int pos = findInsertPosition(event.triggerTime);
			d->scheduledEvents.insert(pos, event);
		}
		// Однократные события просто удаляются (takeFirst уже убрал из списка)
	}
}

int TimeService::findInsertPosition(qint64 triggerTime) const {
	int left = 0;
	int right = d->scheduledEvents.size();

	while (left < right) {
		int mid = left + (right - left) / 2;
		if (d->scheduledEvents[mid].triggerTime < triggerTime) {
			left = mid + 1;
		}
		else {
			right = mid;
		}
	}
	return left;
}
