#include "Content/InventoriesModule/data_providers/entities_data_provider_db.h"
#include "Content/InventoriesModule/data_providers/migrations/inventories_migrations.h"
#include "Content/InventoriesModule/data_providers/readers/entity_reader.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

namespace {
static QString kSql_entitySelectorExpr = R"(
		select 
				e.id, 
				e.name, 
				e.description,
				e.type, 
				e.sub_type, 
				e.icon_path, 
				e.width, 
				e.height,
				e.rarity, 
				e.container_rows, 
				e.container_cols, 
				e.max_stack,
				group_concat(r.resource_id) as resources,
				group_concat(pall.resource_id) as permissions_for_all,
				group_concat(pany.resource_id) as permissions_for_any
		from item_entities e
		left join entity_resource r on r.entity_id = e.id
		left join entity_permissions_all pall on pall.entity_id = e.id 
		left join entity_permissions_any pany on pany.entity_id = e.id
)";
static QString kSql_entityGroupByExpr = R"(
		group by e.id, e.name, e.description, e.type, e.sub_type, 
			e.icon_path, e.width, e.height, e.rarity, 
			e.container_rows, e.container_cols, e.max_stack
)";
}

class EntitiesDataProviderDb::Private {
public:
	Private(EntitiesDataProviderDb* paren) : q(paren) {}
	EntitiesDataProviderDb* q;

	DatabasesService* databasesService;
};

EntitiesDataProviderDb::EntitiesDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

EntitiesDataProviderDb::~EntitiesDataProviderDb() = default;

std::list<std::shared_ptr<ItemEntity>> EntitiesDataProviderDb::entities() const {
	std::list<std::shared_ptr<ItemEntity>> result;
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return result;
	}

	auto reader = conn->executeQuery(QString("%1\n%2")
		.arg(kSql_entitySelectorExpr)
		.arg(kSql_entityGroupByExpr));

	if (!reader) {
		return result;
	}

	while (reader->next()) {
		Inventory::Readers::EntityReader rd;
		auto ent = rd.read(*reader);
		result.push_back(ent);
	}

	return result;
}

std::shared_ptr<ItemEntity> EntitiesDataProviderDb::entity(const QUuid& id) const {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return std::shared_ptr<ItemEntity>();
	}

	auto reader = conn->executeQuery(QString("%1\nwhere e.id = :entity_id\n%2")
		.arg(kSql_entitySelectorExpr)
		.arg(kSql_entityGroupByExpr));

	reader->bindValue(":entity_id", id
		.toString(QUuid::WithoutBraces)
		.toLower());

	if (!reader || !reader->next()) {
		qWarning() << "Can't find entity:" << id;
		return std::shared_ptr<ItemEntity>();
	}

	Inventory::Readers::EntityReader rd;
	return rd.read(*reader);
}
