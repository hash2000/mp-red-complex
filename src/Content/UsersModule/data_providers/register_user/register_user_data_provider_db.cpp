#include "Content/UsersModule/data_providers/register_user/register_user_data_provider_db.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"

#include <QDateTime>

namespace {
static QString kSql_accountInsert = R"(
	insert into accounts (user_hash, username, created_at, last_login)
	values (:user_hash, :username, :created_at, :last_login)
)";
static QString kSql_userHashSelect = R"(
	select user_hash from accounts where username = :username
)";
static QString kSql_lastLoginUpdate = R"(
	update accounts set last_login = :last_login where user_hash = :user_hash
)";
static QString kSql_accountDelete = R"(
	delete from accounts where user_hash = :user_hash
)";
static QString kSql_usersSelect = R"(
	select username from accounts order by last_login desc
)";

}

class RegisterUserDataProviderDb::Private {
public:
	Private(RegisterUserDataProviderDb* parent) : q(parent) {}
	RegisterUserDataProviderDb* q;

	DatabasesService* databasesService;
};

RegisterUserDataProviderDb::RegisterUserDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
}

RegisterUserDataProviderDb::~RegisterUserDataProviderDb() = default;

bool RegisterUserDataProviderDb::addUser(const QString& userHash, const QString& username) {
	auto conn = d->databasesService->connection("accounts");
	if (!conn) {
		return false;
	}

	auto insert = conn->prepare(kSql_accountInsert);
	insert->bindValues({
		{ ":user_hash", userHash },
		{ ":username", username },
		{ ":created_at", QDateTime::currentSecsSinceEpoch() },
		{ ":last_login", QDateTime::currentSecsSinceEpoch() },
		});

	if (!insert->exec()) {
		qWarning() << "Save account failed." << userHash;
		return false;
	}

	return true;
}

QString RegisterUserDataProviderDb::getUserHashByUsername(const QString& username) const {
	auto conn = d->databasesService->connection("accounts");
	if (!conn) {
		return { };
	}
	auto reader = conn->executeQuery(kSql_userHashSelect);
	reader->bindValue(":username", username);

	if (!reader || !reader->next()) {
		qWarning() << "Can't find account:" << username;
		return { };
	}

	const auto hash = reader->value("user_hash")
		.toString();

	return hash;
}

bool RegisterUserDataProviderDb::updateLastLogin(const QString& userHash) {
	auto conn = d->databasesService->connection("accounts");
	if (!conn) {
		return false;
	}

	auto update = conn->prepare(kSql_lastLoginUpdate);
	update->bindValues({
		{ ":last_login", QDateTime::currentSecsSinceEpoch() },
		{ ":username", userHash },
		});

	if (!update->exec()) {
		qWarning() << "Update last login for account failed." << userHash;
		return false;
	}

	return true;
}

bool RegisterUserDataProviderDb::removeUser(const QString& userHash) {
	auto conn = d->databasesService->connection("accounts");
	if (!conn) {
		return false;
	}

	auto deleteRow = conn->prepare(kSql_accountDelete);
	deleteRow->bindValues({
		{ ":user_hash", QDateTime::currentSecsSinceEpoch() },
		});

	if (!deleteRow->exec()) {
		qWarning() << "Delete account failed." << userHash;
		return false;
	}

	return true;
}

QStringList RegisterUserDataProviderDb::listUsernames() const {
	auto conn = d->databasesService->connection("accounts");
	if (!conn) {
		return { };
	}
	auto reader = conn->executeQuery(kSql_usersSelect);

	QStringList usernames;

	while (reader && reader->next()) {
		const auto username = reader->value("username")
			.toString();

		usernames.append(username);
	}

	return usernames;
}
