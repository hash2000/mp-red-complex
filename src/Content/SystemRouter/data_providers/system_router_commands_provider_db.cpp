#include "Content/SystemRouter/data_providers/system_router_commands_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_db.h"

#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"

#include <QDebug>

namespace {

const QString kSqlSelectAll = QStringLiteral(R"(
	select id, version, description, script
	from commands
	order by description collate nocase, id
)");

const QString kSqlSelectById = QStringLiteral(R"(
	select id, version, description, script
	from commands
	where id = :id
)");

const QString kSqlSave = QStringLiteral(R"(
	insert into commands (id, version, description, script)
	values (:id, :version, :description, :script)
	on conflict(id) do update set
		version = excluded.version,
		description = excluded.description,
		script = excluded.script
)");

const QString kSqlDelete = QStringLiteral("delete from commands where id = :id");

} // namespace

class SystemRouterCommandsProviderDb::Private {
public:
	explicit Private(SystemRouterCommandsProviderDb* parent, DatabasesService* databasesService)
		: q(parent)
		, databasesService(databasesService) {
	}

	SystemRouterCommandsProviderDb* q;
	DatabasesService* databasesService = nullptr;
	mutable QString lastError;

	SQLiteConnection* connection() const {
		lastError.clear();
		if (!databasesService) {
			lastError = QStringLiteral("Сервис баз данных не задан");
			return nullptr;
		}

		auto* result = databasesService->connection(SystemRouterDb::kDatabase);
		if (!result) {
			lastError = QStringLiteral("Не удалось открыть хранилище «%1»")
				.arg(QString::fromLatin1(SystemRouterDb::kDatabase));
		}

		return result;
	}

	static SystemRouterCommand readCommand(const std::unique_ptr<SQLiteReader>& reader) {
		SystemRouterCommand command;
		if (!reader) {
			return command;
		}

		command.id = reader->value(QStringLiteral("id")).toString();
		command.version = reader->value(QStringLiteral("version")).toInt();
		command.description = reader->value(QStringLiteral("description")).toString();
		command.script = reader->value(QStringLiteral("script")).toString();
		return command;
	}
};

SystemRouterCommandsProviderDb::SystemRouterCommandsProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this, databasesService)) {
}

SystemRouterCommandsProviderDb::~SystemRouterCommandsProviderDb() = default;

QList<SystemRouterCommand> SystemRouterCommandsProviderDb::commands() const {
	auto* connection = d->connection();
	if (!connection) {
		return {};
	}

	auto reader = connection->executeQuery(kSqlSelectAll);
	if (!reader) {
		d->lastError = connection->lastError();
		return {};
	}

	QList<SystemRouterCommand> result;
	while (reader->next()) {
		result.append(Private::readCommand(reader));
	}

	return result;
}

std::optional<SystemRouterCommand> SystemRouterCommandsProviderDb::command(const QString& id) const {
	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty()) {
		return std::nullopt;
	}

	auto* connection = d->connection();
	if (!connection) {
		return std::nullopt;
	}

	auto reader = connection->prepare(kSqlSelectById);
	if (!reader) {
		d->lastError = connection->lastError();
		return std::nullopt;
	}

	reader->bindValue(QStringLiteral(":id"), normalized);
	if (!reader->next()) {
		return std::nullopt;
	}

	return Private::readCommand(reader);
}

bool SystemRouterCommandsProviderDb::saveCommand(const SystemRouterCommand& command) {
	const auto normalized = normalizeSystemRouterCommandId(command.id);
	if (normalized.isEmpty()) {
		d->lastError = QStringLiteral("Идентификатор команды не задан");
		return false;
	}

	auto* connection = d->connection();
	if (!connection) {
		return false;
	}

	auto statement = connection->prepare(kSqlSave);
	if (!statement) {
		d->lastError = connection->lastError();
		return false;
	}

	statement->bindValue(QStringLiteral(":id"), normalized);
	statement->bindValue(QStringLiteral(":version"), command.version > 0 ? command.version : 1);
	statement->bindValue(QStringLiteral(":description"), command.description);
	statement->bindValue(QStringLiteral(":script"), command.script);

	if (!statement->exec()) {
		d->lastError = connection->lastError();
		qWarning() << "SystemRouterCommandsProviderDb: save failed" << normalized << d->lastError;
		return false;
	}

	return true;
}

bool SystemRouterCommandsProviderDb::removeCommand(const QString& id) {
	const auto normalized = normalizeSystemRouterCommandId(id);
	if (normalized.isEmpty()) {
		return false;
	}

	auto* connection = d->connection();
	if (!connection) {
		return false;
	}

	auto statement = connection->prepare(kSqlDelete);
	if (!statement) {
		d->lastError = connection->lastError();
		return false;
	}

	statement->bindValue(QStringLiteral(":id"), normalized);
	if (!statement->exec()) {
		d->lastError = connection->lastError();
		return false;
	}

	return true;
}

QString SystemRouterCommandsProviderDb::lastError() const {
	return d->lastError;
}
