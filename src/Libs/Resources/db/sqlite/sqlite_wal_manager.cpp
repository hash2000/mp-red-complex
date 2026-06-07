#include "Libs/Resources/db/sqlite/sqlite_wal_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include <QTimer>
#include <QFileInfo>

class SQLiteWalManager::Private {
public:
	Private(SQLiteWalManager* parent)
		: q(parent) {}
	SQLiteWalManager* q;
	SQLiteConnection* conn;
	QTimer timer;
	int maxSizeMB = 10;  // Максимальный размер WAL в мегабайтах
};

SQLiteWalManager::SQLiteWalManager(SQLiteConnection* conn, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {

	connect(&d->timer, &QTimer::timeout, this, &SQLiteWalManager::onCheckpointTimer);
}

SQLiteWalManager::~SQLiteWalManager() = default;

void SQLiteWalManager::startAutoCheckpoint(int intervalMs, int maxSizeMB) {
	d->maxSizeMB = maxSizeMB;
	d->timer.start(intervalMs);  // Запускаем таймер
	qInfo() << "WAL Manager started: check every" << intervalMs << "ms"
		<< "if WAL >" << maxSizeMB << "MB";
}

void SQLiteWalManager::stopAutoCheckpoint() {
	d->timer.stop();
	qInfo() << "WAL Manager stopped";
}

bool SQLiteWalManager::checkpoint(CheckpointMode mode) {
	QString sql;
	switch (mode) {
	case PASSIVE:  sql = "PRAGMA wal_checkpoint(PASSIVE)";  break;
	case FULL:     sql = "PRAGMA wal_checkpoint(FULL)";     break;
	case RESTART:  sql = "PRAGMA wal_checkpoint(RESTART)";  break;
	case TRUNCATE: sql = "PRAGMA wal_checkpoint(TRUNCATE)"; break;
	}

	return d->conn->execute(sql);
}

void SQLiteWalManager::onCheckpointTimer() {
	// Проверяем размер WAL файла
	QString dbPath = d->conn->databaseName();
	QFileInfo walFile(dbPath + "-wal");  // WAL файл: myapp.db-wal

	if (!walFile.exists()) {
		return;  // WAL файла нет - нечего оптимизировать
	}

	qint64 walSize = walFile.size();
	qint64 maxSize = d->maxSizeMB * 1024 * 1024;  // Переводим MB в байты

	if (walSize > maxSize) {
		// WAL файл слишком большой - пора "вклеивать закладки"
		qInfo() << "WAL file too large:" << walSize / 1024 / 1024 << "MB"
			<< "Running PASSIVE checkpoint";

		// Используем PASSIVE чтобы не мешать работе приложения
		checkpoint(PASSIVE);

		// Проверяем результат
		QFileInfo walAfter(dbPath + "-wal");
		if (walAfter.exists()) {
			qInfo() << "After checkpoint:" << walAfter.size() / 1024 / 1024 << "MB";
		}
	}
}
