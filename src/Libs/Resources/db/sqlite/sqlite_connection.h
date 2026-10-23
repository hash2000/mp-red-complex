#pragma once
#include <QObject>
#include <QString>
#include <QVariant>
#include <memory>

class SQLiteReader;
typedef struct sqlite3 sqlite3;

class SQLiteConnection : public QObject {
	Q_OBJECT
public:
	SQLiteConnection(QObject* parent = nullptr);
	~SQLiteConnection();

	// Открытие/закрытие соединения
	bool open(const QString& connectionString);
	void close();
	bool isOpen() const;

	// Выполнение запросов
	std::unique_ptr<SQLiteReader> executeQuery(const QString& sql);
	std::unique_ptr<SQLiteReader> prepare(const QString& sql);

	// Простое выполнение без возврата данных
	bool execute(const QString& sql);

	// Транзакции
	bool beginTransaction();
	bool commit();
	bool rollback();

	// Информация о соединении
	QString databaseName() const;
	QString lastError() const;

	// Настройки
	void setPragma(const QString& pragma, const QString& value);

	sqlite3* handle() const;

private:
	friend class SQLiteReader;
	class Private;
	std::unique_ptr<Private> d;
};
