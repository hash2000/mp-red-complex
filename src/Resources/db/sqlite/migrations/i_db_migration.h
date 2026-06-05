#pragma once

class SQLiteConntection;

class IDbMigration {
public:
	virtual bool migrate(SQLiteConntection* db) const = 0;
};
