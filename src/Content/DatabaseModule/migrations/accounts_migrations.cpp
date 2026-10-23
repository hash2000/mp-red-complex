#include "Content/DatabaseModule/migrations/accounts_migrations.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"

namespace AccountsMigrations {
void build(MigrationManager* migrator) {
	migrator->addMigration(1, "Initialized database",
		[](SQLiteConnection& db) -> bool {
			return db.execute(R"(
CREATE TABLE accounts (
    user_hash TEXT PRIMARY KEY,           -- SHA-256 от masterSeed (имя папки)
    username TEXT NOT NULL UNIQUE,        -- Логин для входа
    created_at INTEGER NOT NULL,          -- Unix timestamp
    last_login INTEGER                    -- Unix timestamp
);

CREATE INDEX idx_accounts_username ON accounts(username);
		)");
	},
		[](SQLiteConnection& db) -> bool {
			return db.execute(R"(
DROP TABLE IF EXISTS accounts;
		)");
	});

}
};
