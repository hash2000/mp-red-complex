#pragma once
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
	bool execQuesry();
	bool prepare(const QString& sql);

	// Биндинг параметров
	void bindValue(int index, const QVariant& value);
	void bindValue(const QString& name, const QVariant& value);
	void bindValues(const QVariantMap& values);
	void bindValues(const std::initializer_list<std::pair<QString, QVariant>>& values);
	void bindValuesList(const QVariantList& values);
	void clearBindings();

	// Навигация
	bool next();

	// Получение данных
	QVariant value(int column) const;
	QVariant value(const QString& name) const;

	template<typename T>
	std::optional<T> getValue(int column) const {
		QVariant val = value(column);
		if (val.isNull() || !val.canConvert<T>()) {
			return std::nullopt;
		}
		return val.value<T>();
	}

	template<typename T>
	std::optional<T> getValue(const QString& name) const {
		QVariant val = value(name);
		if (val.isNull() || !val.canConvert<T>()) {
			return std::nullopt;
		}
		return val.value<T>();
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

	// Сброс
	void reset();
	void finish();

private:
	class Private;
	std::unique_ptr<Private> d;
};
