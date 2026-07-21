#include "Content/DatabaseModule/migrations/fetch_api_migrations.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"

namespace FetchApiMigrations {
void build(MigrationManager* migrator) {
	migrator->addMigration(1, "Initialized database",
		[](SQLiteConnection& db) -> bool {
		return db.execute(R"(
CREATE TABLE "queries_tree" (
	"id" INTEGER NOT null PRIMARY KEY AUTOINCREMENT,
	"parent_id" INTEGER,
	"name" TEXT NOT NULL,
	CONSTRAINT FK_queries_tree_parent_id FOREIGN KEY (parent_id) REFERENCES queries_tree(id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE "queries" (
	"queries_tree_id" INTEGER NOT NULL,
	"method" TEXT NOT NULL,
	"request" TEXT NOT NULL,
	"parameters" TEXT,
	"headers" TEXT,
	"body" TEXT,
	CONSTRAINT "PK_queries" PRIMARY KEY (queries_tree_id),
	CONSTRAINT "FK_queries_queries_tree" FOREIGN KEY ("queries_tree_id") REFERENCES "queries_tree"("id")
);

CREATE INDEX IF NOT EXISTS IX_queries_tree_parent_id ON queries_tree(parent_id);

		)");
	},
		[](SQLiteConnection& db) -> bool {
		return db.execute(R"(
DROP TABLE IF EXISTS "queries";
DROP TABLE IF EXISTS "queries_tree";
		)");
	});

}
};
