#include "Resources/db/sqlite/sqlite_reader.h"
#include "Resources/db/sqlite/sqlite_connection.h"
#include <QDebug>

class SQLiteReader::Private {
public:
	Private(SQLiteConnection& conn, SQLiteReader* parent)
		: q(parent)
		, connection(conn)
		, query(conn.dataBase()) {
	}
	SQLiteReader* q;

	SQLiteConnection& connection;
	QSqlQuery query;
	bool hasValidRecord = false;
};

SQLiteReader::SQLiteReader(SQLiteConnection& connection)
	: d(std::make_unique<Private>(connection, this)) {
}

SQLiteReader::~SQLiteReader() = default;

QSqlQuery& SQLiteReader::query() {
	return d->query;
}

const QSqlQuery& SQLiteReader::query() const {
	return d->query;
}

bool SQLiteReader::exec(const QString& sql) {
	d->hasValidRecord = false;
	if (!d->query.exec(sql)) {
		qWarning() << "Query execution error:" << d->query.lastError().text();
		return false;
	}
	return true;
}

bool SQLiteReader::prepare(const QString& sql) {
	d->hasValidRecord = false;
	if (!d->query.prepare(sql)) {
		qWarning() << "Query preparation error:" << d->query.lastError().text();
		return false;
	}
	return true;
}

void SQLiteReader::bindValue(const QString& placeholder, const QVariant& value, QSql::ParamType type) {
	d->query.bindValue(placeholder, value, type);
}

void SQLiteReader::bindValue(int pos, const QVariant& value, QSql::ParamType type) {
	d->query.bindValue(pos, value, type);
}

void SQLiteReader::addBindValue(const QVariant& value, QSql::ParamType type) {
	d->query.addBindValue(value, type);
}

void SQLiteReader::bindValues(const QVariantMap& values) {
	for (auto it = values.constBegin(); it != values.constEnd(); it++) {
		// Если ключ начинается с ':', то используем как есть
		// Иначе добавляем ':'
		QString placeholder = it.key();
		if (!placeholder.startsWith(':')) {
			placeholder.prepend(':');
		}
		d->query.bindValue(placeholder, it.value());
	}
}

// Удобный метод с initializer_list
void SQLiteReader::bindValues(const std::initializer_list<std::pair<QString, QVariant>>& values) {

	for (const auto& [key, value] : values) {
		QString placeholder = key;
		if (!placeholder.startsWith(':')) {
			placeholder.prepend(':');
		}
		d->query.bindValue(placeholder, value);
	}
}

// Позиционные параметры из списка
void SQLiteReader::bindValuesList(const QVariantList& values) {
	for (int i = 0; i < values.size(); ++i) {
		d->query.bindValue(i, values[i]);
	}
}

bool SQLiteReader::next() {
	d->hasValidRecord = d->query.next();
	return d->hasValidRecord;
}

bool SQLiteReader::seek(int index, bool relative) {
	d->hasValidRecord = d->query.seek(index, relative);
	return d->hasValidRecord;
}

bool SQLiteReader::first() {
	d->hasValidRecord = d->query.first();
	return d->hasValidRecord;
}

bool SQLiteReader::last() {
	d->hasValidRecord = d->query.last();
	return d->hasValidRecord;
}

bool SQLiteReader::previous() {
	d->hasValidRecord = d->query.previous();
	return d->hasValidRecord;
}

QSqlRecord SQLiteReader::current() const {
	if (!d->hasValidRecord) {
		throw std::runtime_error("No current record available");
	}
	return d->query.record();
}

std::optional<QSqlRecord> SQLiteReader::optionalCurrent() const {
	if (!d->hasValidRecord) {
		return std::nullopt;
	}
	return d->query.record();
}

QVariant SQLiteReader::value(int column) const {
	return d->query.value(column);
}

QVariant SQLiteReader::value(const QString& name) const {
	return d->query.value(name);
}

bool SQLiteReader::isNull(int column) const {
	return d->query.value(column).isNull();
}

bool SQLiteReader::isNull(const QString& name) const {
	return d->query.value(name).isNull();
}

int SQLiteReader::columnCount() const {
	return d->query.record().count();
}

QString SQLiteReader::columnName(int index) const {
	return d->query.record().fieldName(index);
}

int SQLiteReader::columnIndex(const QString& name) const {
	return d->query.record().indexOf(name);
}

bool SQLiteReader::isValid() const {
	return d->query.isValid();
}

bool SQLiteReader::isActive() const {
	return d->query.isActive();
}

bool SQLiteReader::isSelect() const {
	return d->query.isSelect();
}

int SQLiteReader::size() const {
	return d->query.size();
}

int SQLiteReader::at() const {
	return d->query.at();
}

void SQLiteReader::finish() {
	d->query.finish();
	d->hasValidRecord = false;
}

bool SQLiteReader::reset() {
	d->hasValidRecord = false;
	return d->query.exec();
}
