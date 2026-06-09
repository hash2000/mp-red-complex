#include "Libs/Resources/db/sqlite/sqlite_reader.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include <QDebug>
#include <sqlite3.h>

class SQLiteReader::Private {
public:
	Private(SQLiteConnection& conn, SQLiteReader* parent)
		: q(parent)
		, connection(conn) {
	}
	SQLiteReader* q;

	SQLiteConnection& connection;
	sqlite3_stmt* stmt = nullptr;
	bool hasRow = false;
	QStringList columnNames;
	bool firstNext = true;

	void cacheColumnNames();
	QVariant extractValue(int column) const;
	int findColumn(const QString& name) const;
};

SQLiteReader::SQLiteReader(SQLiteConnection& connection)
	: d(std::make_unique<Private>(connection, this)) {
}

SQLiteReader::~SQLiteReader() = default;

bool SQLiteReader::exec(const QString& sql) {
	finish();

	int rc = sqlite3_prepare_v2(
		d->connection.handle(),
		sql.toUtf8().constData(),
		-1,
		&d->stmt,
		nullptr
	);

	if (rc != SQLITE_OK) {
		qWarning() << "Failed to prepare statement:" << d->connection.lastError();
		return false;
	}

	d->firstNext = true;
	return true;
}

bool SQLiteReader::exec() {
	if (!d->stmt) {
		qWarning() << "No prepared statement";
		return false;
	}

	reset();
	d->firstNext = true;
	return true;
}

bool SQLiteReader::prepare(const QString& sql) {
	finish();

	int rc = sqlite3_prepare_v2(
		d->connection.handle(),
		sql.toUtf8().constData(),
		-1,
		&d->stmt,
		nullptr
	);

	if (rc != SQLITE_OK) {
		qWarning() << "Failed to prepare statement:" << d->connection.lastError();
		return false;
	}

	return true;
}

void SQLiteReader::bindValue(int index, const QVariant& value) {
	if (!d->stmt) {
		return;
	}

	int rc = SQLITE_OK;

	if (value.isNull()) {
		rc = sqlite3_bind_null(d->stmt, index + 1);
	}
	else {

		switch (value.type()) {
		case QVariant::Int:
		case QVariant::LongLong:
		rc = sqlite3_bind_int64(d->stmt, index + 1, value.toLongLong());
		break;

		case QVariant::Double:
		rc = sqlite3_bind_double(d->stmt, index + 1, value.toDouble());
		break;

		case QVariant::String: {
			QByteArray utf8 = value.toString().toUtf8();
			rc = sqlite3_bind_text(d->stmt, index + 1,
				utf8.constData(), utf8.size(),
				SQLITE_TRANSIENT);
			break;
		}

		case QVariant::ByteArray: {
			QByteArray data = value.toByteArray();
			rc = sqlite3_bind_blob(d->stmt, index + 1,
				data.constData(), data.size(),
				SQLITE_TRANSIENT);
			break;
		}

		case QVariant::Bool:
		rc = sqlite3_bind_int(d->stmt, index + 1, value.toBool() ? 1 : 0);
		break;

		default:
		qWarning() << "Unsupported bind type:" << value.type();
		break;
		}
	}

	if (rc != SQLITE_OK) {
		qWarning() << "Failed to bind value at index" << index
			<< ":" << d->connection.lastError();
	}
}

void SQLiteReader::bindValue(const QString& name, const QVariant& value) {
	if (!d->stmt) {
		return;
	}

	QByteArray nameUtf8 = name.toUtf8();
	if (!nameUtf8.startsWith(':')) {
		nameUtf8.prepend(':');
	}

	int index = sqlite3_bind_parameter_index(d->stmt, nameUtf8.constData());
	if (index == 0) {
		qWarning() << "Parameter not found:" << name;
		return;
	}

	bindValue(index - 1, value);  // SQLite использует 1-based индексы
}

void SQLiteReader::bindValues(const QVariantMap& values) {
	for (auto it = values.constBegin(); it != values.constEnd(); it++) {
		// Если ключ начинается с ':', то используем как есть
		// Иначе добавляем ':'
		QString placeholder = it.key();
		if (!placeholder.startsWith(':')) {
			placeholder.prepend(':');
		}

		bindValue(placeholder, it.value());
	}	
}

void SQLiteReader::bindValues(const std::initializer_list<std::pair<QString, QVariant>>& values) {
	for (const auto& [key, value] : values) {
		QString placeholder = key;
		if (!placeholder.startsWith(':')) {
			placeholder.prepend(':');
		}

		bindValue(placeholder, value);
	}
}

void SQLiteReader::bindValuesList(const QVariantList& values) {
	for (int i = 0; i < values.size(); ++i) {
		bindValue(i, values[i]);
	}
}

void SQLiteReader::clearBindings() {
	if (d->stmt) {
		sqlite3_clear_bindings(d->stmt);
	}
}

bool SQLiteReader::next() {
	if (!d->stmt) {
		return false;
	}

	int rc = sqlite3_step(d->stmt);

	if (rc == SQLITE_ROW) {
		d->hasRow = true;
		if (d->firstNext) {
			d->cacheColumnNames();
			d->firstNext = false;
		}
		return true;
	}
	else if (rc == SQLITE_DONE) {
		d->hasRow = false;
		return false;
	}
	else {
		qWarning() << "Step error:" << d->connection.lastError();
		d->hasRow = false;
		return false;
	}
}

QVariant SQLiteReader::value(int column) const {
	if (!d->hasRow || !d->stmt) {
		return QVariant();
	}

	return d->extractValue(column);
}

QVariant SQLiteReader::value(const QString& name) const {
	return value(d->findColumn(name));
}

void SQLiteReader::Private::cacheColumnNames() {
	columnNames.clear();
	int count = sqlite3_column_count(stmt);
	columnNames.reserve(count);

	for (int i = 0; i < count; ++i) {
		const char* name = sqlite3_column_name(stmt, i);
		columnNames.append(name ? QString::fromUtf8(name) : QString());
	}
}

QVariant SQLiteReader::Private::extractValue(int column) const {
	int type = sqlite3_column_type(stmt, column);

	switch (type) {
	case SQLITE_INTEGER:
	return QVariant::fromValue(sqlite3_column_int64(stmt, column));

	case SQLITE_FLOAT:
	return sqlite3_column_double(stmt, column);

	case SQLITE_TEXT: {
		const unsigned char* text = sqlite3_column_text(stmt, column);
		int size = sqlite3_column_bytes(stmt, column);
		return QString::fromUtf8(reinterpret_cast<const char*>(text), size);
	}

	case SQLITE_BLOB: {
		const void* blob = sqlite3_column_blob(stmt, column);
		int size = sqlite3_column_bytes(stmt, column);
		return QByteArray(static_cast<const char*>(blob), size);
	}

	case SQLITE_NULL:
	default:
	return QVariant();
	}
}

int SQLiteReader::Private::findColumn(const QString& name) const {
	int index = columnNames.indexOf(name);
	if (index == -1) {
		// Поиск без учета регистра
		for (int i = 0; i < columnNames.size(); ++i) {
			if (columnNames[i].compare(name, Qt::CaseInsensitive) == 0) {
				return i;
			}
		}
		throw std::runtime_error("Column not found: " + name.toStdString());
	}
	return index;
}

bool SQLiteReader::isNull(int column) const {
	if (!d->stmt) return true;
	return sqlite3_column_type(d->stmt, column) == SQLITE_NULL;
}

bool SQLiteReader::isNull(const QString& name) const {
	return isNull(d->findColumn(name));
}

int SQLiteReader::columnCount() const {
	return d->stmt ? sqlite3_column_count(d->stmt) : 0;
}

QString SQLiteReader::columnName(int index) const {
	if (index >= 0 && index < d->columnNames.size()) {
		return d->columnNames[index];
	}
	return QString();
}

int SQLiteReader::columnIndex(const QString& name) const {
	return d->findColumn(name);
}

bool SQLiteReader::isValid() const {
	return d->stmt != nullptr && d->hasRow;
}

bool SQLiteReader::isActive() const {
	return d->stmt != nullptr;
}

void SQLiteReader::reset() {
	if (d->stmt) {
		sqlite3_reset(d->stmt);
		d->hasRow = false;
		d->firstNext = true;
	}
}

void SQLiteReader::finish() {
	if (d->stmt) {
		sqlite3_finalize(d->stmt);
		d->stmt = nullptr;
		d->hasRow = false;
		d->columnNames.clear();
	}
}
