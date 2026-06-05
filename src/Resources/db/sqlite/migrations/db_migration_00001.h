#pragma once
#include "Resources/db/sqlite/migrations/i_db_migration.h"

class DbMigration_00001 : public IDbMigration {
public:
	bool migrate(SQLiteConntection* db) const override;
};
