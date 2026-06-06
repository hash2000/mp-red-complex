#include "Resources/db/sqlite/migration.h"

Migration::Migration(int version, const QString& description)
	: _version(version)
	, _description(description) {
}

SimpleMigration::SimpleMigration(int version, const QString& description, MigrationFunc up, MigrationFunc down)
	: Migration(version, description)
	, _up(std::move(up))
	, _down(std::move(down)) {
}

bool SimpleMigration::up(SQLiteConnection& db) {
	return _up ? _up(db) : false;
}

bool SimpleMigration::down(SQLiteConnection& db) {
	return _down ? _down(db) : false;
}
