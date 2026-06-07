#pragma once
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QString>
#include <QVariantMap>

#include <optional>
#include <memory>

class SQLiteConnection;

class SQLiteReader {
public:
	explicit SQLiteReader(SQLiteConnection& connection);
	~SQLiteReader();

	// Выполнение запроса
	bool exec(const QString& sql);
	bool exec();
	bool prepare(const QString& sql);

	// Биндинг параметров
	void bindValue(const QString& placeholder, const QVariant& value, QSql::ParamType type = QSql::In);
	void bindValue(int pos, const QVariant& value, QSql::ParamType type = QSql::In);
	void addBindValue(const QVariant& value, QSql::ParamType type = QSql::In);

	// Вспомогательный метод для биндинга map (Qt 6 совместимый)
	void bindValues(const QVariantMap& values);

	// Именованные параметры с префиксом (удобный синтаксис)
	void bindValues(const std::initializer_list<std::pair<QString, QVariant>>& values);

	// Позиционные параметры из списка
	void bindValuesList(const QVariantList& values);

	// Навигация
	bool next();
	bool seek(int index, bool relative = false);
	bool first();
	bool last();
	bool previous();

	// Получение данных
	QSqlRecord current() const;
	std::optional<QSqlRecord> optionalCurrent() const;

	// Удобные методы для получения значений
	QVariant value(int column) const;
	QVariant value(const QString& name) const;

	template<typename T>
	std::optional<T> getValue(int column) const {
		if (!isValid() || isNull(column)) {
			return std::nullopt;
		}
		QVariant val = value(column);
		if (val.canConvert<T>()) {
			return val.value<T>();
		}
		return std::nullopt;
	}

	template<typename T>
	std::optional<T> getValue(const QString& name) const {
		if (!isValid() || isNull(name)) {
			return std::nullopt;
		}
		QVariant val = value(name);
		if (val.canConvert<T>()) {
			return val.value<T>();
		}
		return std::nullopt;
	}

	// Проверка на NULL
	bool isNull(int column) const;
	bool isNull(const QString& name) const;

	// Информация о запросе
	int columnCount() const;
	QString columnName(int index) const;
	int columnIndex(const QString& name) const;

	// Состояние
	bool isValid() const;
	bool isActive() const;
	bool isSelect() const;
	int size() const;
	int at() const;

	// Сброс
	void finish();
	bool reset();

	// Получить сырой QSqlQuery (если нужно)
	QSqlQuery& query();
	const QSqlQuery& query() const;

private:
	class Private;
	std::unique_ptr<Private> d;
};
