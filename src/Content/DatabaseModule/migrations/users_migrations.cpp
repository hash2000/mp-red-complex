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

				-- Таблица существующих разрешений
				CREATE TABLE permissions (
					id TEXT NOT NULL,
					CONSTRAINT PK_permissions PRIMARY KEY (id)
				);

				INSERT INTO permissions (id) VALUES('--users-view');
				INSERT INTO permissions (id) VALUES('--users-create');
				INSERT INTO permissions (id) VALUES('--user-can-register-in-game');
				INSERT INTO permissions (id) VALUES('--user-can-register-in-messages');
				INSERT INTO permissions (id) VALUES('--characters-view');

				-- Таблица разрешений пользователя
				CREATE TABLE users_permission (
					user_id TEXT NOT NULL,
					permission TEXT NOT NULL,
					is_active INTEGER DEFAULT (1) NOT NULL,
					CONSTRAINT PK_users_permission PRIMARY KEY (user_id, permission),
					CONSTRAINT FK_users_permission_users FOREIGN KEY (user_id) REFERENCES users(id),
					CONSTRAINT FK_users_permission_permissions FOREIGN KEY (permission) REFERENCES permissions(id)
				);
		)");
	},
		[](SQLiteConnection& db) -> bool {
			return db.execute(R"(
				DROP TABLE IF EXISTS users_permission;
				DROP TABLE IF EXISTS users;
				DROP TABLE IF EXISTS permissions;
		)");
	});

}
};
