CREATE TABLE IF NOT EXISTS "menu_items" (
	"id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	"parent_id" INTEGER,
	"name" TEXT NOT NULL,
	"data" TEXT NOT NULL DEFAULT '{}',
	CONSTRAINT "FK_menu_items_parent_id" FOREIGN KEY (parent_id) REFERENCES "menu_items"(id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE INDEX IF NOT EXISTS IX_menu_items_parent_id ON menu_items(parent_id);
