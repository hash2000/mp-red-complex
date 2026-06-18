#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"


namespace InventoriesMigrations {
void build(MigrationManager* migrator) {
	migrator->addMigration(1, "Initialized database",
		[](SQLiteConnection& db) -> bool {
		return db.execute(R"(
			-- Таблица типов сущностей
			-- нужна для проверки на уровне базы, что тип у сущности указан верно
			CREATE TABLE "item_entity_types" (
				id TEXT NOT NULL,
				CONSTRAINT PK_item_entity_types PRIMARY KEY (id)
			);

			-- Таблица подтипов
			-- нужна для проверки на уровне базы, что например тип экипировки у сущности указан 
			CREATE TABLE "item_entity_sub_types" (
				id TEXT NOT NULL,
				CONSTRAINT PK_item_entity_sub_type PRIMARY KEY (id)
			);

			-- Таблица ресурсов
			-- нужна для проверки на уровне базы, что ресурс у сущности указан верно
			CREATE TABLE "resources" (
				id TEXT NOT NULL,
				CONSTRAINT PK_resources PRIMARY KEY (id)
			);

			-- Таблица сущностей предметов
			CREATE TABLE "item_entities" (
				id TEXT NOT NULL,
				name TEXT NOT NULL,
				description TEXT NOT NULL,
				"type" TEXT NOT NULL,
				sub_type TEXT,
				icon_path TEXT NOT NULL,
				width INTEGER DEFAULT (1) NOT NULL,
				height INTEGER DEFAULT (1) NOT NULL,
				rarity INTEGER DEFAULT (1) NOT NULL,
				container_rows INTEGER,
				container_cols INTEGER,
				max_stack INTEGER DEFAULT (1) NOT NULL,
				CONSTRAINT PK_game_items PRIMARY KEY (id),
				CONSTRAINT FK_item_entities_item_entity_types FOREIGN KEY ("type") REFERENCES item_entity_types(id),
				CONSTRAINT FK_item_entities_item_entity_sub_type FOREIGN KEY (sub_type) REFERENCES "item_entity_sub_types"(id)
			);

			-- Привязка сущности к ресурсу
			-- для типа - resource
			CREATE TABLE "entity_resource" (
				entity_id TEXT NOT NULL,
				resource_id TEXT NOT NULL,
				CONSTRAINT PK_entity_resource PRIMARY KEY (entity_id,resource_id),
				CONSTRAINT FK_entity_resource_entities FOREIGN KEY (entity_id) REFERENCES "item_entities"(id) ON DELETE CASCADE ON UPDATE CASCADE,
				CONSTRAINT FK_entity_resource_resources FOREIGN KEY (resource_id) REFERENCES resources(id)
			);

			-- Таблицы entity_permissions_all и entity_permissions_any указывают разрешиения для предмета
			-- добавлять в него другие предметы определённого типа
			-- для типа - container
			CREATE TABLE "entity_permissions_all" (
				entity_id TEXT NOT NULL,
				resource_id TEXT NOT NULL,
				CONSTRAINT PK_entity_permissions_all PRIMARY KEY (entity_id,resource_id),
				CONSTRAINT FK_entity_permissions_all_entities FOREIGN KEY (entity_id) REFERENCES "item_entities"(id),
				CONSTRAINT FK_entity_permissions_all_resources FOREIGN KEY (resource_id) REFERENCES resources(id)
			);

			CREATE TABLE "entity_permissions_any" (
				entity_id TEXT NOT NULL,
				resource_id TEXT NOT NULL,
				CONSTRAINT PK_entity_permissions_any PRIMARY KEY (entity_id,resource_id),
				CONSTRAINT FK_entity_permissions_any_entities FOREIGN KEY (entity_id) REFERENCES "item_entities"(id),
				CONSTRAINT FK_entity_permissions_any_resources FOREIGN KEY (resource_id) REFERENCES resources(id)
			);

			CREATE INDEX IDX_entity_permissions_all_entity_id ON entity_permissions_all (entity_id);
			CREATE INDEX IDX_entity_permissions_any_entity_id ON entity_permissions_any (entity_id);

			-- Таблица созданных предметов
			CREATE TABLE items (
				id TEXT NOT NULL,
				entity_id TEXT NOT NULL,
				container_id TEXT,
				count INTEGER NOT NULL,
				position_base INTEGER NOT NULL, 
				position_secondary INTEGER NOT NULL,
				item_level INTEGER DEFAULT (1) NOT NULL,
				ia_active INTEGER DEFAULT (1) NOT NULL, 
				created_at TEXT DEFAULT (datetime('now')) NOT NULL, 
				CONSTRAINT PK_items PRIMARY KEY (id),

				CONSTRAINT FK_items_entities FOREIGN KEY (entity_id) REFERENCES "item_entities"(id),
				CONSTRAINT FK_items_containers FOREIGN KEY (container_id) REFERENCES containers(item_id)
			);

			CREATE UNIQUE INDEX UQ_items_id_container_id ON items (id,container_id);
			CREATE INDEX UQ_items_container_id ON items (container_id);

			-- Таблица контейнеров - склады, рюкзаки, экипировка
			-- для типа - container
			CREATE TABLE containers (
				item_id TEXT NOT NULL,
				name TEXT NOT NULL,
				CONSTRAINT PK_equipments PRIMARY KEY (item_id),
				CONSTRAINT FK_containers_items FOREIGN KEY (item_id) REFERENCES items(id)
			);
		)");
	},
		[](SQLiteConnection& db) -> bool {
		return db.execute(R"(
			DROP TABLE IF EXISTS items;
			DROP TABLE IF EXISTS entity_permissions_any;
			DROP TABLE IF EXISTS entity_permissions_all;
			DROP TABLE IF EXISTS entity_resource;
			DROP TABLE IF EXISTS item_entities;
			DROP TABLE IF EXISTS resources;
			DROP TABLE IF EXISTS item_entity_equipment_type;
			DROP TABLE IF EXISTS item_entity_types;
			DROP TABLE IF EXISTS containers;
		)");
	});

}
};
