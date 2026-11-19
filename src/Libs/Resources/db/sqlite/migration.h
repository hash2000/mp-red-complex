#pragma once

#include <QString>
#include <QVersionNumber>
#include <functional>

class SQLiteConnection;

class Migration {
public:
	Migration(qint64 version, const QString& description);
	virtual ~Migration() = default;

	qint64 version() const {
		return _version;
	}
	QString description() const {
		return _description;
	}

	virtual bool up(SQLiteConnection& db) = 0;
	virtual bool down(SQLiteConnection& db) = 0;

private:
	qint64 _version;
	QString _description;
};

class SimpleMigration : public Migration {
public:
	using MigrationFunc = std::function<bool(SQLiteConnection&)>;

	SimpleMigration(qint64 version, const QString& description, MigrationFunc up, MigrationFunc down = nullptr);

	bool up(SQLiteConnection& db) override;
	bool down(SQLiteConnection& db) override;

private:
	MigrationFunc _up;
	MigrationFunc _down;
};
