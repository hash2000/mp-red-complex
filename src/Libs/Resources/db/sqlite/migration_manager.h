#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <memory>
#include <map>
#include <functional>

class SQLiteConnection;
class Migration;

class MigrationManager : public QObject {
	Q_OBJECT

public:
	explicit MigrationManager(QObject* parent = nullptr);
	~MigrationManager() override;

	// Регистрация миграций
	void addMigration(std::unique_ptr<Migration> migration);
	void addMigration(qint64 version, const QString& description,
		std::function<bool(SQLiteConnection&)> up,
		std::function<bool(SQLiteConnection&)> down = nullptr);

	// Создание таблицы миграций (если её нет)
	bool initializeSchema(SQLiteConnection& db);

	// Запуск миграций до последней версии
	bool migrate(SQLiteConnection& db, qint64 targetVersion = -1);

	// Откат миграций
	bool rollback(SQLiteConnection& db, int steps = 1);
	bool rollbackTo(SQLiteConnection& db, qint64 version);

	// Информация о миграциях
	qint64 currentVersion(SQLiteConnection& db) const;
	qint64 latestVersion() const;
	QStringList migrationHistory(SQLiteConnection& db) const;
	bool needsMigration(SQLiteConnection& db) const;

signals:
	void migrationStarted(qint64 version, const QString& description);
	void migrationCompleted(qint64 version, const QString& description);
	void migrationFailed(qint64 version, const QString& description, const QString& error);

private:
	struct MigrationRecord {
		qint64 version;
		QString description;
		QDateTime appliedAt;
	};

	bool applyMigration(SQLiteConnection& db, Migration* migration);
	bool revertMigration(SQLiteConnection& db, Migration* migration);
	QList<MigrationRecord> getAppliedMigrations(SQLiteConnection& db) const;
	bool recordMigration(SQLiteConnection& db, Migration* migration);
	bool removeMigrationRecord(SQLiteConnection& db, qint64 version);

private:
	class Private;
	std::unique_ptr<Private> d;

};
