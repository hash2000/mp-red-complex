#include "Content/UsersModule/data_providers/migrations/users_migrations.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"

namespace UsersMigrations {
void build(MigrationManager* migrator) {
	migrator->addMigration(1, "Initialized database",
		[](SQLiteConnection& db) -> bool {
			return db.execute(R"(
            -- Таблица пользователей
            CREATE TABLE IF NOT EXISTS users (
								-- login-hash = id
                id TEXT PRIMARY KEY NOT NULL UNIQUE,
                display_name TEXT NOT NULL UNIQUE,
                password_hash TEXT NOT NULL,
                chest_id TEXT NOT NULL UNIQUE,
                icon_path TEXT NOT NULL,
								is_active INTEGER DEFAULT (1) NOT NULL,
                created_at TEXT DEFAULT (datetime('now'))
            );

						-- Таблица персонажей
						CREATE TABLE characters (
								id TEXT(54) PRIMARY KEY NOT NULL,
								name TEXT(16) NOT NULL,
								equipment_id TEXT(54) NOT NULL,
								icon_path TEXT NOT NULL,
								char_level INTEGER DEFAULT (1) NOT NULL,
								is_active INTEGER DEFAULT (1) NOT NULL
						);

            -- Таблица персонажей пользователей
            CREATE TABLE IF NOT EXISTS user_characters (
								users_id TEXT NOT NULL,
								character_id TEXT NOT NULL,
								created_at TEXT DEFAULT (datetime('now')),

								-- Внешний ключ на users.id
								FOREIGN KEY (character_id) REFERENCES "characters"(id) 
										ON DELETE CASCADE 
										ON UPDATE CASCADE,

								-- Внешний ключ на users.id
								FOREIGN KEY (users_id) REFERENCES users(id) 
										ON DELETE CASCADE 
										ON UPDATE CASCADE,
            
								-- Уникальный ключ на обе колонки (составной ключ)
								UNIQUE(users_id, character_id)
            );

            -- Индексы для быстрого поиска
            CREATE INDEX IF NOT EXISTS idx_user_characters_user_id 
                ON user_characters(users_id);
            CREATE INDEX IF NOT EXISTS idx_user_characters_character_id 
                ON user_characters(character_id);
		)");
	},
		[](SQLiteConnection& db) -> bool {
			return db.execute(R"(
						DROP TABLE IF EXISTS users;
						DROP TABLE IF EXISTS user_characters;
						DROP TABLE IF EXISTS characters;
		)");
	});

}
};
