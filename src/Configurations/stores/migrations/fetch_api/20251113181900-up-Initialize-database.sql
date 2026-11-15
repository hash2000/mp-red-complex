CREATE TABLE "queries" (
	"id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	"parent_id" INTEGER,
	"name" TEXT NOT NULL,
	"data" TEXT NOT NULL,
	CONSTRAINT "FK_queries_parent_id" FOREIGN KEY (parent_id) REFERENCES "queries"(id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX IF NOT EXISTS IX_queries_parent_id ON queries(parent_id);
