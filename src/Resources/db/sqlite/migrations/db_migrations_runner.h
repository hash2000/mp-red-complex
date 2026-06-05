#pragma once
class SQLiteConntection;

class DbMigrationsRunner {
public:
	void run(SQLiteConntection* db);
};
