#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/migration.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"


class MigrationManager::Private {
public:
	Private(MigrationManager* parent)
		: q(parent) {}
	MigrationManager* q;

	std::map<int, std::unique_ptr<Migration>> migrations;
};

MigrationManager::MigrationManager(QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
}

MigrationManager::~MigrationManager() = default;

void MigrationManager::addMigration(std::unique_ptr<Migration> migration) {
	int version = migration->version();
	d->migrations[version] = std::move(migration);
}

void MigrationManager::addMigration(int version, const QString& description,
	std::function<bool(SQLiteConnection&)> up,
	std::function<bool(SQLiteConnection&)> down) {
	addMigration(std::make_unique<SimpleMigration>(version, description, up, down));
}

bool MigrationManager::initializeSchema(SQLiteConnection& db) {
	return db.execute(R"(
        CREATE TABLE IF NOT EXISTS _migrations (
            version INTEGER PRIMARY KEY,
            description TEXT NOT NULL,
            applied_at TEXT NOT NULL DEFAULT (datetime('now'))
        )
    )");
}

bool MigrationManager::migrate(SQLiteConnection& db, int targetVersion) {
	if (!initializeSchema(db)) {
		qCritical() << "Failed to initialize migration schema";
		return false;
	}

	int current = currentVersion(db);
	int target = targetVersion >= 0 ? targetVersion : latestVersion();

	qInfo() << "Current DB version:" << current << ", Target version:" << target;

	if (current == target) {
		qInfo() << "Database is up to date";
		return true;
	}

	if (current > target) {
		qWarning() << "Database version is higher than target, use rollback instead";
		return false;
	}

	// Применяем миграции по порядку
	for (auto& [version, migration] : d->migrations) {
		if (version <= current) {
			continue;
		}

		if (version > target) {
			break;
		}

		qInfo() << "Applying migration" << version << ":" << migration->description();
		emit migrationStarted(version, migration->description());

		if (!applyMigration(db, migration.get())) {
			QString error = QString("Failed to apply migration %1: %2")
				.arg(version)
				.arg(migration->description());
			qCritical() << error;
			emit migrationFailed(version, migration->description(), error);
			return false;
		}

		recordMigration(db, migration.get());
		emit migrationCompleted(version, migration->description());
		qInfo() << "Migration" << version << migration->description() << "applied successfully";
	}

	return true;
}

bool MigrationManager::rollback(SQLiteConnection& db, int steps) {
	auto applied = getAppliedMigrations(db);
	if (applied.isEmpty()) {
		qInfo() << "No migrations to rollback";
		return true;
	}

	// Сортируем в обратном порядке
	std::sort(applied.begin(), applied.end(),
		[](const MigrationRecord& a, const MigrationRecord& b) {
		return a.version > b.version;
	});

	int rolled = 0;
	for (const auto& record : applied) {
		if (rolled >= steps) {
			break;
		}

		auto it = d->migrations.find(record.version);
		if (it == d->migrations.end()) {
			qWarning() << "Migration" << record.version << "not found in registry";
			continue;
		}

		qInfo() << "Rolling back migration" << record.version;

		if (!revertMigration(db, it->second.get())) {
			qCritical() << "Failed to rollback migration" << record.version;
			return false;
		}

		removeMigrationRecord(db, record.version);
		rolled++;
	}

	return true;
}

bool MigrationManager::rollbackTo(SQLiteConnection& db, int version) {
	int current = currentVersion(db);
	int steps = current - version;
	return rollback(db, steps);
}

int MigrationManager::currentVersion(SQLiteConnection& db) const {
	auto reader = db.executeQuery(
		"SELECT MAX(version) as version FROM _migrations"
	);

	if (reader && reader->next()) {
		return reader->value("version").toInt();
	}

	return 0;
}

int MigrationManager::latestVersion() const {
	if (d->migrations.empty()) {
		return 0;
	}
	return d->migrations.rbegin()->first;
}

QStringList MigrationManager::migrationHistory(SQLiteConnection& db) const {
	QStringList history;

	auto reader = db.executeQuery(
		"SELECT version, description, applied_at FROM _migrations ORDER BY version"
	);

	while (reader && reader->next()) {
		history << QString("v%1: %2 (applied at %3)")
			.arg(reader->value("version").toInt())
			.arg(reader->value("description").toString())
			.arg(reader->value("applied_at").toString());
	}

	return history;
}

bool MigrationManager::needsMigration(SQLiteConnection& db) const {
	return currentVersion(db) < latestVersion();
}

bool MigrationManager::applyMigration(SQLiteConnection& db, Migration* migration) {
	db.beginTransaction();

	try {
		if (!migration->up(db)) {
			db.rollback();
			return false;
		}
		db.commit();
		return true;
	}
	catch (const std::exception& e) {
		qCritical() << "Migration failed:" << e.what();
		db.rollback();
		return false;
	}
}

bool MigrationManager::revertMigration(SQLiteConnection& db, Migration* migration) {
	db.beginTransaction();

	try {
		if (!migration->down(db)) {
			db.rollback();
			return false;
		}
		db.commit();
		return true;
	}
	catch (const std::exception& e) {
		qCritical() << "Rollback failed:" << e.what();
		db.rollback();
		return false;
	}
}

QList<MigrationManager::MigrationRecord> MigrationManager::getAppliedMigrations(SQLiteConnection& db) const {
	QList<MigrationRecord> records;

	auto reader = db.executeQuery(
		"SELECT version, description, applied_at FROM _migrations ORDER BY version"
	);

	while (reader && reader->next()) {
		MigrationRecord mr;
		mr.version = reader->value("version").toInt();
		mr.description = reader->value("description").toString();
		mr.appliedAt = QDateTime::fromString(
			reader->value("applied_at").toString(),
			Qt::ISODate
		);
		records.append(mr);
	}

	return records;
}

void MigrationManager::recordMigration(SQLiteConnection& db, Migration* migration) {
	auto insert = db.prepare(
		"INSERT INTO _migrations (version, description) VALUES (:version, :description)"
	);
	insert->bindValues({
		{ ":version", migration->version() },
		{ ":description", migration->description() }
		});
	insert->exec();
}

void MigrationManager::removeMigrationRecord(SQLiteConnection& db, int version) {
	auto del = db.prepare("DELETE FROM _migrations WHERE version = :version");
	del->bindValue(":version", version);
	del->exec();
}
