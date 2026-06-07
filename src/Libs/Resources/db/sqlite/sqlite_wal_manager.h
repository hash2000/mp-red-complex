#pragma once
#include <QObject>
#include <memory>

class SQLiteConnection;

class SQLiteWalManager : public QObject {
	Q_OBJECT
public:
	SQLiteWalManager(SQLiteConnection* conn, QObject* parent = nullptr);
	~SQLiteWalManager() override;

	// Автоматический checkpoint по таймеру
	void startAutoCheckpoint(int intervalMs = 60000, int maxSizeMB = 10);
	void stopAutoCheckpoint();

	// Ручной checkpoint
	enum CheckpointMode {
		PASSIVE,   // Без блокировок (для работы "на лету")
		FULL,      // Полный (для планового обслуживания)
		RESTART,   // Полный + сброс (перед бэкапом)
		TRUNCATE   // Полный + обрезание (перед закрытием приложения)
	};

	bool checkpoint(CheckpointMode mode = PASSIVE);

private slots:
	void onCheckpointTimer();  // Вызывается по таймеру

private:
	class Private;
	std::unique_ptr<Private> d;
};
