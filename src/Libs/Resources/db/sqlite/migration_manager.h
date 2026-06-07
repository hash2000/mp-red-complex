#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QDateTime>
#include <memory>
#include <map>

class SQLiteConnection;
class Migration;

class MigrationManager : public QObject {
	Q_OBJECT

public:
	explicit MigrationManager(QObject* parent = nullptr);
	~MigrationManager() override;

	// Регистрация миграций
	void addMigration(std::unique_ptr<Migration> migration);
	void addMigration(int version, const QString& description,
		std::function<bool(SQLiteConnection&)> up,
		std::function<bool(SQLiteConnection&)> down = nullptr);

	// Создание таблицы миграций (если её нет)
	bool initializeSchema(SQLiteConnection& db);

	// Запуск миграций до последней версии
	bool migrate(SQLiteConnection& db, int targetVersion = -1);

	// Откат миграций
	bool rollback(SQLiteConnection& db, int steps = 1);
	bool rollbackTo(SQLiteConnection& db, int version);

	// Информация о миграциях
	int currentVersion(SQLiteConnection& db) const;
	int latestVersion() const;
	QStringList migrationHistory(SQLiteConnection& db) const;
	bool needsMigration(SQLiteConnection& db) const;

signals:
	void migrationStarted(int version, const QString& description);
	void migrationCompleted(int version, const QString& description);
	void migrationFailed(int version, const QString& description, const QString& error);

private:
	struct MigrationRecord {
		int version;
		QString description;
		QDateTime appliedAt;
	};

	bool applyMigration(SQLiteConnection& db, Migration* migration);
	bool revertMigration(SQLiteConnection& db, Migration* migration);
	QList<MigrationRecord> getAppliedMigrations(SQLiteConnection& db) const;
	void recordMigration(SQLiteConnection& db, Migration* migration);
	void removeMigrationRecord(SQLiteConnection& db, int version);

private:
	class Private;
	std::unique_ptr<Private> d;

};
