#include "Resources/db/sqlite/migrations/db_migrations_runner.h"
#include "Resources/db/sqlite/migrations/db_migration_00001.h"
#include "Resources/db/sqlite/sqlite_connection.h"

#include <QString>

#include <vector>
#include <memory>
#include <mutex>

void DbMigrationsRunner::run(SQLiteConntection* db) {

	//static std::once_flag _flag;
	//std::call_once(_flag, [db]
	//{
	//	std::vector<std::unique_ptr<IDbMigration>> migrations;
	//	migrations.push_back(std::make_unique<DbMigration_00001>());

	//	int index = 1;
	//	for (const auto& migration : migrations) {
	//		const auto name = QString("migrations_%1").arg(index, 5, 10, QChar('0'));
	//		db->execute()
	//		
	//		if (!migration->migrate(db)) {
	//			continue;
	//		}



	//		index++;
	//	}
	//});
}
