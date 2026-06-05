#include "Resources/db/sqlite/sqlite_connection.h"
#include "Resources/db/sqlite/sqlite_reader.h"
#include "Resources/db/sqlite/migrations/db_migrations_runner.h"
#include "Resources/resources.h"
#include <QUuid>
#include <QDebug>
#include <QDir>

class SQLiteConnection::Private {
public:
	Private(SQLiteConnection* parent) : q(parent) {}
	SQLiteConnection* q;

	Resources* resources;
	QSqlDatabase db;
	QString connectionName;
	bool isOpen = false;
};

SQLiteConnection::SQLiteConnection(Resources* resources, QObject* parent)
	: QObject(parent)
	, d(std::make_unique<Private>(this)) {
	d->resources = resources;
	d->connectionName = QString("sqlite_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

SQLiteConnection::~SQLiteConnection() {
	close();
}

bool SQLiteConnection::open(const QString& name) {
	const auto path = d->resources->Variables.get("Resources.Path", "").toString();
	if (path.isNull()) {
		qWarning() << "Embedded database is not avalible.";
		return false;
	}

	QDir dir(path);
	const auto dbPath = dir.filePath("data/" + name);

	d->db = QSqlDatabase::addDatabase("QSQLITE", d->connectionName);
	d->db.setDatabaseName(dbPath);

	if (!d->db.open()) {
		qWarning() << "Failed to open database:" << d->db.lastError().text();
		return false;
	}

	// WAL режим для лучшей конкурентности
	QSqlQuery query(d->db);
	query.exec("PRAGMA journal_mode=WAL");
	query.exec("PRAGMA synchronous=NORMAL");
	query.exec("PRAGMA busy_timeout=5000");
	query.exec("PRAGMA foreign_keys=ON");

	d->isOpen = true;
	return true;
}

void SQLiteConnection::close() {
	if (!d->db.isOpen()) {
		return;
	}

	d->db.close();
	d->isOpen = false;
}

bool SQLiteConnection::isOpen() const {
	return d->isOpen;
}

std::unique_ptr<SQLiteReader> SQLiteConnection::executeQuery(const QString& sql) {
	auto reader = std::make_unique<SQLiteReader>(*this);
	if (!reader->exec(sql)) {
		throw std::runtime_error(lastError().toStdString());
	}
	return reader;
}

std::unique_ptr<SQLiteReader> SQLiteConnection::prepare(const QString& sql) {
	auto reader = std::make_unique<SQLiteReader>(*this);
	if (!reader->prepare(sql)) {
		throw std::runtime_error(lastError().toStdString());
	}
	return reader;
}

bool SQLiteConnection::execute(const QString& sql) {
	QSqlQuery query(d->db);
	if (!query.exec(sql)) {
		qWarning() << "SQL error:" << query.lastError().text();
		return false;
	}
	return true;
}

bool SQLiteConnection::beginTransaction() {
	return d->db.transaction();
}

bool SQLiteConnection::commit() {
	return d->db.commit();
}

bool SQLiteConnection::rollback() {
	return d->db.rollback();
}

QString SQLiteConnection::databaseName() const {
	return d->db.databaseName();
}

QString SQLiteConnection::lastError() const {
	return d->db.lastError().text();
}

QSqlDatabase SQLiteConnection::dataBase() {
	return d->db;
}

void SQLiteConnection::setPragma(const QString& pragma, const QString& value) {
	QSqlQuery query(d->db);
	query.exec(QString("PRAGMA %1 = %2").arg(pragma, value));
}
