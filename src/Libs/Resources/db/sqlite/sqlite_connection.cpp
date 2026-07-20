#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"

#include <sqlite3.h>
#include <optional>

namespace {
struct ConnectionProperties {
	QString path;
	QString key;
};
}

class SQLiteConnection::Private {
public:
	Private(SQLiteConnection* parent) : q(parent) {}
	SQLiteConnection* q;

	sqlite3* db = nullptr;
	QString dbPath;
	bool isOpen = false;

	static std::optional<ConnectionProperties> parseConnectionString(const QString& connectionString);
};

SQLiteConnection::SQLiteConnection(QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
}

SQLiteConnection::~SQLiteConnection() {
	close();
}

bool SQLiteConnection::open(const QString& connectionString) {
	const auto connectionOptions = Private::parseConnectionString(connectionString);
	if (!connectionOptions) {
		qWarning() << "Bad connection string.";
		return false;
	}

	d->dbPath = connectionOptions.value().path;

	int rc = sqlite3_open_v2(
		d->dbPath.toUtf8().constData(),
		&d->db,
		SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX,
		nullptr
	);

	if (rc != SQLITE_OK) {
		qCritical()
			<< "Failed to open database:" << d->dbPath
			<< "Error:" << (d->db ? sqlite3_errmsg(d->db) : "Unknown");

		if (d->db) {
			sqlite3_close(d->db);
			d->db = nullptr;
		}

		return false;
	}

	// WAL режим для лучшей конкурентности
	execute("PRAGMA journal_mode=WAL");
	execute("PRAGMA synchronous=NORMAL");
	execute("PRAGMA busy_timeout=5000");
	execute("PRAGMA foreign_keys=ON");

	if (!connectionOptions.value().key.isEmpty()) {
		auto setupDbKeyResult = execute(QString("PRAGMA fPRAGMA key = x'%1'")
			.arg(connectionOptions.value().key));
		if (!setupDbKeyResult) {
			qWarning() << "Bad Database key.";
			return false;
		}
	}

	d->isOpen = true;
	qInfo() << "Database opened:" << d->dbPath;

	return true;
}

void SQLiteConnection::close() {
	if (!d->isOpen || !d->db) {
		return;
	}

	// Оптимизация перед закрытием
	execute("PRAGMA optimize");
	execute("PRAGMA wal_checkpoint(TRUNCATE)");

	// Закрываем базу
	sqlite3_close_v2(d->db);
	d->db = nullptr;
	d->isOpen = false;

	qInfo() << "Database closed:" << d->dbPath;
	d->dbPath.clear();
}

bool SQLiteConnection::isOpen() const {
	return d->isOpen && d->db != nullptr;
}

std::unique_ptr<SQLiteReader> SQLiteConnection::executeQuery(const QString& sql) {
	auto reader = std::make_unique<SQLiteReader>(*this);
	if (!reader->exec(sql)) {
		qCritical() << "Execute reader error:" << lastError().toStdString();
		return std::unique_ptr<SQLiteReader>();
	}
	return reader;
}

std::unique_ptr<SQLiteReader> SQLiteConnection::prepare(const QString& sql) {
	auto reader = std::make_unique<SQLiteReader>(*this);
	if (!reader->prepare(sql)) {
		qCritical() << "Execute reader error:" << lastError().toStdString();
		return std::unique_ptr<SQLiteReader>();
	}
	return reader;
}

bool SQLiteConnection::execute(const QString& sql) {
	if (!d->db) {
		qWarning() << "Database not open";
		return false;
	}

	char* errMsg = nullptr;
	int rc = sqlite3_exec(d->db, sql.toUtf8().constData(), nullptr, nullptr, &errMsg);

	if (rc != SQLITE_OK) {
		qWarning() << "SQL error:" << (errMsg ? errMsg : "Unknown error");
		if (errMsg) {
			sqlite3_free(errMsg);
		}
		return false;
	}

	return true;
}

bool SQLiteConnection::beginTransaction() {
	return execute("BEGIN TRANSACTION");
}

bool SQLiteConnection::commit() {
	return execute("COMMIT");
}

bool SQLiteConnection::rollback() {
	return execute("ROLLBACK");
}

QString SQLiteConnection::databaseName() const {
	return d->dbPath;
}

QString SQLiteConnection::lastError() const {
	if (!d->db) {
		return "Database not open";
	}
	return QString::fromUtf8(sqlite3_errmsg(d->db));
}

void SQLiteConnection::setPragma(const QString& pragma, const QString& value) {
	execute(QString("PRAGMA %1 = %2").arg(pragma, value));
}

sqlite3* SQLiteConnection::handle() const {
	return d->db;
}

std::optional<ConnectionProperties> SQLiteConnection::Private::parseConnectionString(const QString& connectionString) {
	if (connectionString.isEmpty()) {
		return std::nullopt;
	}

	ConnectionProperties result;
	const auto connectionOptions = connectionString.split(";", Qt::SkipEmptyParts);
	for (const auto option : connectionOptions) {
		const auto kv = option.split("=", Qt::SkipEmptyParts);
		if (kv.count() != 2) {
			continue;
		}

		const auto paramName = kv[0].toLower();
		if (paramName == "file") {
			result.path = kv[1].toLower();
		}
		else if (paramName == "token") {
			result.key = kv[1].toLower();
		}
	}

	if (result.path.isEmpty()) {
		return std::nullopt;
	}

	return result;
}
