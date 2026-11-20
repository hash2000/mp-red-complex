#include "Content/DatabaseModule/data_providers/data_blobs_provider_db.h"

#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"

#include <QDebug>

class DataBlobsProviderDb::Private {
public:
	Private(DataBlobsProviderDb* parent, DatabasesService* databasesService,
		const QString& databaseName, const QString& tableName)
		: q(parent)
		, databasesService(databasesService)
		, databaseName(databaseName)
		, tableName(tableName) {
		const auto quoted = quoteIdentifier(tableName);
		sqlSelectAll = QStringLiteral(
			"select id, version, description, data from %1 "
			"order by description collate nocase, id").arg(quoted);
		sqlSelectById = QStringLiteral(
			"select id, version, description, data from %1 where id = :id").arg(quoted);
		sqlSave = QStringLiteral(
			"insert into %1 (id, version, description, data) "
			"values (:id, :version, :description, :data) "
			"on conflict(id) do update set "
			"version = excluded.version, description = excluded.description, data = excluded.data")
			.arg(quoted);
		sqlDelete = QStringLiteral("delete from %1 where id = :id").arg(quoted);
	}

	DataBlobsProviderDb* q;
	DatabasesService* databasesService = nullptr;
	QString databaseName;
	QString tableName;
	QString sqlSelectAll;
	QString sqlSelectById;
	QString sqlSave;
	QString sqlDelete;
	mutable QString lastError;

	static QString quoteIdentifier(const QString& identifier) {
		QString quoted = identifier;
		quoted.replace(QLatin1Char('"'), QStringLiteral("\"\""));
		return QStringLiteral("\"%1\"").arg(quoted);
	}

	SQLiteConnection* connection() const {
		lastError.clear();
		if (!databasesService) {
			lastError = QStringLiteral("Сервис баз данных не задан");
			return nullptr;
		}

		auto* result = databasesService->connection(databaseName);
		if (!result) {
			lastError = QStringLiteral("Не удалось открыть хранилище «%1»").arg(databaseName);
		}

		return result;
	}

	static DataBlob readBlob(const std::unique_ptr<SQLiteReader>& reader) {
		DataBlob blob;
		if (!reader) {
			return blob;
		}

		blob.id = reader->value(QStringLiteral("id")).toString();
		blob.version = reader->value(QStringLiteral("version")).toInt();
		blob.description = reader->value(QStringLiteral("description")).toString();
		blob.data = reader->value(QStringLiteral("data")).toString();
		return blob;
	}
};

DataBlobsProviderDb::DataBlobsProviderDb(DatabasesService* databasesService,
	const QString& databaseName, const QString& tableName)
	: d(std::make_unique<Private>(this, databasesService, databaseName, tableName)) {
}

DataBlobsProviderDb::~DataBlobsProviderDb() = default;

QList<DataBlob> DataBlobsProviderDb::blobs() const {
	auto* connection = d->connection();
	if (!connection) {
		return {};
	}

	auto reader = connection->executeQuery(d->sqlSelectAll);
	if (!reader) {
		d->lastError = connection->lastError();
		return {};
	}

	QList<DataBlob> result;
	while (reader->next()) {
		result.append(Private::readBlob(reader));
	}

	return result;
}

std::optional<DataBlob> DataBlobsProviderDb::blob(const QString& id) const {
	if (id.trimmed().isEmpty()) {
		return std::nullopt;
	}

	auto* connection = d->connection();
	if (!connection) {
		return std::nullopt;
	}

	auto reader = connection->prepare(d->sqlSelectById);
	if (!reader) {
		d->lastError = connection->lastError();
		return std::nullopt;
	}

	reader->bindValue(QStringLiteral(":id"), id.trimmed());
	if (!reader->next()) {
		return std::nullopt;
	}

	return Private::readBlob(reader);
}

bool DataBlobsProviderDb::saveBlob(const DataBlob& blob) {
	const auto id = blob.id.trimmed();
	if (id.isEmpty()) {
		d->lastError = QStringLiteral("Идентификатор блока не задан");
		return false;
	}

	auto* connection = d->connection();
	if (!connection) {
		return false;
	}

	auto statement = connection->prepare(d->sqlSave);
	if (!statement) {
		d->lastError = connection->lastError();
		return false;
	}

	statement->bindValue(QStringLiteral(":id"), id);
	statement->bindValue(QStringLiteral(":version"), blob.version > 0 ? blob.version : 1);
	statement->bindValue(QStringLiteral(":description"), blob.description);
	statement->bindValue(QStringLiteral(":data"), blob.data);

	if (!statement->exec()) {
		d->lastError = connection->lastError();
		qWarning() << "DataBlobsProviderDb: save failed" << id << d->lastError;
		return false;
	}

	return true;
}

bool DataBlobsProviderDb::removeBlob(const QString& id) {
	if (id.trimmed().isEmpty()) {
		return false;
	}

	auto* connection = d->connection();
	if (!connection) {
		return false;
	}

	auto statement = connection->prepare(d->sqlDelete);
	if (!statement) {
		d->lastError = connection->lastError();
		return false;
	}

	statement->bindValue(QStringLiteral(":id"), id.trimmed());
	if (!statement->exec()) {
		d->lastError = connection->lastError();
		return false;
	}

	return true;
}

QString DataBlobsProviderDb::lastError() const {
	return d->lastError;
}
