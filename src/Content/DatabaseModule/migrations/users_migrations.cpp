#include "Content/DatabaseModule/migrations/users_migrations.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"

namespace UsersMigrations {
void build(MigrationManager* migrator) {
	migrator->addMigration(1, "Initialized database",
		[](SQLiteConnection& db) -> bool {
			return db.execute(R"(
         -- Таблица пользователей
				CREATE TABLE users (
					id TEXT NOT NULL,
					display_name TEXT NOT NULL,
					password_hash TEXT NOT NULL,
					icon_path TEXT NOT NULL,
					is_active INTEGER DEFAULT (1) NOT NULL,
					created_at TEXT DEFAULT (datetime('now')),
					CONSTRAINT USERS_PK PRIMARY KEY (id)
				);
		)");
	},
		[](SQLiteConnection& db) -> bool {
			return db.execute(R"(
				DROP TABLE IF EXISTS users;
		)");
	});

}
};
