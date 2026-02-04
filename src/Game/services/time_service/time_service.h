#pragma once
#include "Game/services/time_service/time_events.h"
#include <QObject>
#include <memory>

class TimeService : public QObject {
	Q_OBJECT
public:
	explicit TimeService(QObject* parent = nullptr);
	~TimeService() override;

	// Управление основным таймером
	void start();
	void stop();
	bool isRunning() const;

	qint64 elapsedMilliseconds() const;
	QDateTime currentTime() const;

	// Создание однократного таймера (вызовется один раз через задержку)
	void singleShot(int ms, const QString& timerId);

	// Создание повторяющегося таймера
	void startTimer(int intervalMs, const QString& timerId);
	bool stopTimer(const QString& timerId);
	bool hasTimer(const QString& timerId) const;

private:
	void checkAndTriggerEvents(qint64 currentTime);

	// Бинарный поиск для эффективной вставки (опционально для больших списков)
	int findInsertPosition(qint64 triggerTime) const;

private slots:
	void onMainTimerTimeout();

signals:
	void tick(const TickEvent& event);
	void timer(const TimerEvent& event);
	void paused();
	void resumed();

private:
	class Private;
	std::unique_ptr<Private> d;
};
