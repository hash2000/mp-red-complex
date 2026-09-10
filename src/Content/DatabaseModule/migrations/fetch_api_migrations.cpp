#include "Content/DatabaseModule/migrations/fetch_api_migrations.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"

namespace FetchApiMigrations {
void build(MigrationManager* migrator) {
	migrator->addMigration(1, "Initialized database",
		[](SQLiteConnection& db) -> bool {
		return db.execute(R"(
CREATE TABLE "queries_tree" (
	"id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	"parent_id" INTEGER,
	"name" TEXT NOT NULL,
	"data" TEXT NOT NULL,
	CONSTRAINT "FK_queries_parent_id" FOREIGN KEY (parent_id) REFERENCES "queries"(id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX IF NOT EXISTS IX_queries_parent_id ON queries(parent_id);

		)");
	},
		[](SQLiteConnection& db) -> bool {
		return db.execute(R"(
DROP TABLE IF EXISTS "queries";
		)");
	});

}
};
