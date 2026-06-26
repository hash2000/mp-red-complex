#include "Content/UsersModule/data_providers/users/users_data_provider_db.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/UsersModule/models/user.h"
#include "Libs/Resources/db/sqlite/migration_manager.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"
#include <QUuid>

namespace {
static QString kSql_userSelect = R"(
		select id, display_name, password_hash, icon_path, created_at from users where is_active = 1
)";
static QString kSql_userInsert = R"(
		insert into users (id, display_name, password_hash, icon_path, created_at)
		values(:id, :displayName, :password, :iconPath, 1, datetime('now'));
)";
static QString kSql_userDelete = R"(
		update users set is_active = 0
		where id = :login;
)";
static QString kSql_userPermissionsSelect = R"(
		select permission from users_permission where is_active = 1
)";
}

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
}

UsersDataProviderDb::~UsersDataProviderDb() = default;

bool UsersDataProviderDb::containsUser(const QString& loginHash) {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return false;
	}

	auto reader = conn->executeQuery(kSql_userSelect);
	reader->bindValue(":login", loginHash);

	if (!reader || !reader->next()) {
		return false;
	}

	return true;
}

std::shared_ptr<UserData> UsersDataProviderDb::loadUser(const QString& loginHash) const {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return std::shared_ptr<UserData>();
	}

	auto reader = conn->executeQuery(QString("%1 and id = :login").arg(kSql_userSelect));
	reader->bindValue(":login", loginHash);

	if (!reader || !reader->next()) {
		qWarning() << "Can't find user:" << loginHash << "Maybe is inactive";
		return std::shared_ptr<UserData>();
	}

	auto result = std::make_shared<UserData>();
	result->loginHash = reader->value("id").toString();
	result->displayName = reader->value("display_name").toString();
	result->iconPath = reader->value("icon_path").toString();
	result->passwordHash = reader->value("password_hash").toString();

	auto date = reader->value("created_at").toString();
	if (!date.isEmpty()) {
		result->createdAt = QDateTime::fromString(date, "yyyy-MM-dd HH:mm:ss");
	}

	return result;
}

std::set<QString> UsersDataProviderDb::permissions(const QString& id) const {
	std::set<QString> result;
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return result;
	}

	auto reader = conn->executeQuery(QString("%1 and user_id = :user_id")
		.arg(kSql_userPermissionsSelect));

	reader->bindValue(":user_id", id);

	while (reader->next()) {
		result.emplace(reader->value("permission").toString());
	}

	return result;
}

bool UsersDataProviderDb::saveUser(const std::shared_ptr<UserData> user) {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return false;
	}

	auto insert = conn->prepare(kSql_userInsert);
	insert->bindValues({
		{ ":id", user->loginHash },
		{ ":displayName", user->displayName },
		{ ":password", user->passwordHash },
		{ ":iconPath", user->iconPath },
		});

	if (!insert->exec()) {
		qWarning() << "Save user failed." << user->displayName;
		return false;
	}

	return true;
}

bool UsersDataProviderDb::deleteUser(const QString& loginHash) {
	auto conn = d->databasesService->connection("users");
	if (!conn) {
		return false;
	}

	auto reader = conn->prepare(kSql_userDelete);
	reader->bindValue(":login", loginHash);

	if (!reader || !reader->exec()) {
		qWarning() << "Can't delete user:" << loginHash;
		return false;
	}

	return true;
}
