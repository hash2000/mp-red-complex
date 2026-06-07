#pragma once
#include <QObject>
#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QSqlField>
#include <QVariant>
#include <memory>

class Resources;
class SQLiteReader;

class SQLiteConnection : public QObject {
	Q_OBJECT
public:
	SQLiteConnection(Resources* resources, QObject* parent = nullptr);
	~SQLiteConnection();

	// Открытие/закрытие соединения
	bool open(const QString& name);
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

private:
	QSqlDatabase dataBase();

private:
	friend class SQLiteReader;
	class Private;
	std::unique_ptr<Private> d;
};
