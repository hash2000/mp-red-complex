#include "Content/UsersModule/data_providers/users/users_data_provider_db.h"
#include "Content/UsersModule/services/users_service.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/UsersModule/models/user/user.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include <QUuid>

class UsersDataProviderDb::Private {
public:
	Private(UsersDataProviderDb* parent)
		: q(parent) {}

	UsersDataProviderDb* q;
	DatabasesService* databasesService;
};

UsersDataProviderDb::UsersDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("users");
	migrator->addMigration(1, "Initialized database",
	[](SQLiteConnection& db) -> bool{
		return db.execute(R"(
            -- Таблица пользователей
            CREATE TABLE IF NOT EXISTS users (
                id TEXT PRIMARY KEY NOT NULL UNIQUE,
                display_name TEXT NOT NULL UNIQUE,
                login TEXT NOT NULL,
                password_hash TEXT NOT NULL,
                email TEXT NOT NULL UNIQUE,
                chest_id TEXT NOT NULL UNIQUE,
                icon_path TEXT NOT NULL,
                created_at TEXT DEFAULT (datetime('now'))
            );

            -- Таблица персонажей пользователей
            CREATE TABLE IF NOT EXISTS user_characters (
                users_id TEXT NOT NULL,
                character_id TEXT NOT NULL,
                created_at TEXT DEFAULT (datetime('now')),
                
                -- Внешний ключ на users.id
                FOREIGN KEY (users_id) REFERENCES users(id) 
                    ON DELETE CASCADE 
                    ON UPDATE CASCADE,
                
                -- Уникальный ключ на обе колонки (составной ключ)
                UNIQUE(users_id, character_id)
            );

            -- Индексы для быстрого поиска
            CREATE INDEX IF NOT EXISTS idx_users_login ON users(login);
            CREATE INDEX IF NOT EXISTS idx_users_email ON users(email);
            CREATE INDEX IF NOT EXISTS idx_user_characters_user_id 
                ON user_characters(users_id);
            CREATE INDEX IF NOT EXISTS idx_user_characters_character_id 
                ON user_characters(character_id);
		)");
	},
	[](SQLiteConnection& db) -> bool {
		return db.execute(R"(
						DROP TABLE IF EXISTS user_characters;
						DROP TABLE IF EXISTS users;
		)");
	});
}

UsersDataProviderDb::~UsersDataProviderDb() = default;

std::optional<UserData> UsersDataProviderDb::loadUser(const QString& loginHash) const {
	auto connection = d->databasesService->connection("users");
	if (!connection) {
		return std::nullopt;
	}


	return std::nullopt;
}

bool UsersDataProviderDb::saveUser(const UserData& user) {
	return true;
}

bool UsersDataProviderDb::deleteUser(const QString& loginHash) {
	return true;
}
