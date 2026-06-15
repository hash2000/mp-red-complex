#include "Content/UsersModule/data_providers/users/users_data_provider_db.h"
#include "Content/UsersModule/data_providers/migrations/users_migrations.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/UsersModule/models/user/user.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"
#include <QUuid>

class UsersDataProviderDb::Private {
public:
	Private(UsersDataProviderDb* parent)
		: q(parent) {}

	UsersDataProviderDb* q;
	DatabasesService* databasesService;

	bool loadCharacters(UserData& data);
	bool saveCharacters(const UserData& data);
};

UsersDataProviderDb::UsersDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("users");
	UsersMigrations::build(migrator);
}

UsersDataProviderDb::~UsersDataProviderDb() = default;

std::optional<UserData> UsersDataProviderDb::loadUser(const QString& loginHash) const {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return std::nullopt;
	}

	auto reader = conn->executeQuery(R"(
			select * from users u	where u.id = :login and is_active = 1
	)");

	reader->bindValue(":login", loginHash);

	if (!reader || !reader->next()) {
		qWarning() << "Can't find user:" << loginHash << "Maybe is inactive";
		return std::nullopt;
	}

	UserData result;
	result.loginHash = reader->value("id").toString();
	result.displayName = reader->value("display_name").toString();
	result.iconPath = reader->value("icon_path").toString();
	result.chestId = QUuid::fromString(reader->value("chest_id").toString());
	result.passwordHash = reader->value("password_hash").toString();
	d->loadCharacters(result);

	return result;
}

bool UsersDataProviderDb::Private::loadCharacters(UserData& data) {
	auto conn = databasesService->connection("users");
	if (!conn) {
		return false;
	}

	auto reader = conn->executeQuery(R"(
			select character_id from user_characters c where c.users_id = :login
	)");

	if (!reader) {
		return false;
	}

	reader->bindValue(":login", data.loginHash);

	while (reader->next()) {
		auto charId = QUuid(reader->value("character_id").toString());
		data.characters.push_back(charId);
	}

	return true;
}

bool UsersDataProviderDb::Private::saveCharacters(const UserData& data) {
	auto conn = databasesService->connection("users");
	if (!conn) {
		return false;
	}

	QStringList items;
	for (const auto& chr : data.characters) {
		items << QString("('%1', %2, datetime('now'))").arg(data.loginHash, chr.toString(QUuid::WithoutBraces));
	}
	QString valuesExpr = items.join(",");
	QString query = QString(R"(
		insert into user_characters (users_id, character_id, created_at, is_active)
		select * from (
				values 
					%1
		) as t(users_id, character_id, created_at)
		on conflict(users_id, character_id)
		do update set
				updated_at = datetime('now'),
				is_active = 1;
	)").arg(valuesExpr);

	return conn->execute(query);
}

bool UsersDataProviderDb::saveUser(const UserData& user) {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return false;
	}

	if (!conn->beginTransaction()) {
		qWarning() << "Coud't open transaction when saving user." << user.displayName;
		return false;
	}

	auto insert = conn->prepare(R"(
			insert into users (id, display_name, password_hash, chest_id, icon_path, is_active, created_at)
			values(:id, :displayName, :password, :chestId, :iconPath, 1, datetime('now'));
	)");

	insert->bindValues({
		{ ":id", user.loginHash },
		{ ":displayName", user.displayName },
		{ ":password", user.passwordHash },
		{ ":iconPath", user.iconPath },
		});

	if (!insert->exec()) {
		qWarning() << "Save user failed." << user.displayName;
		conn->rollback();
		return false;
	}

	if (!d->saveCharacters(user)) {
		qWarning() << "Save user failed." << user.displayName << "Save characters.";
		conn->rollback();
		return false;
	}

	conn->commit();
	return true;
}

bool UsersDataProviderDb::deleteUser(const QString& loginHash) {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return false;
	}

	auto reader = conn->prepare(R"(
			update users set is_active = 0
			where id = :login;
	)");

	reader->bindValue(":login", loginHash);

	if (!reader || !reader->exec()) {
		qWarning() << "Can't delete user:" << loginHash;
		return false;
	}

	return true;
}
