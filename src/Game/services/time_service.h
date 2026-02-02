#pragma once
#include <QObject>
#include <memory>

class EventBus;

class TimeService : public QObject {
	Q_OBJECT

public:
	explicit TimeService(EventBus* eventBus, QObject* parent = nullptr);
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
	void publishTick(qint64 elapsed, double deltaTime);
	void checkAndTriggerEvents(qint64 currentTime);
	void publishSecond(qint64 elapsed);
	void publishMinute(qint64 elapsed);

	// Бинарный поиск для эффективной вставки (опционально для больших списков)
	int findInsertPosition(qint64 triggerTime) const;

private slots:
	void onMainTimerTimeout();

signals:
	void tick(qint64 elapsed, double deltaTime);
	void second(int count);
	void minute(int count);

private:
	class Private;
	std::unique_ptr<Private> d;
};
