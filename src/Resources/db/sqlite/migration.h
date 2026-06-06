#pragma once

#include <QString>
#include <QVersionNumber>
#include <functional>

class SQLiteConnection;

class Migration {
public:
	Migration(int version, const QString& description);
	virtual ~Migration() = default;

	int version() const {
		return _version;
	}
	QString description() const {
		return _description;
	}

	virtual bool up(SQLiteConnection& db) = 0;
	virtual bool down(SQLiteConnection& db) = 0;

private:
	int _version;
	QString _description;
};

class SimpleMigration : public Migration {
public:
	using MigrationFunc = std::function<bool(SQLiteConnection&)>;

	SimpleMigration(int version, const QString& description, MigrationFunc up, MigrationFunc down = nullptr);

	bool up(SQLiteConnection& db) override;
	bool down(SQLiteConnection& db) override;

private:
	MigrationFunc _up;
	MigrationFunc _down;
};
