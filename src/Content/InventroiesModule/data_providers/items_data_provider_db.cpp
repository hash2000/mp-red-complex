#include "Content/InventroiesModule/data_providers/items_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"
#include "Content/InventroiesModule/data_providers/readers/item_reader.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

namespace {
static QString kSql_itemSelect = R"(
		select id, entity_id, container_id, item_level level, count, position_base, position_secondary
		from items
		where ia_active = 1
)";
static QString kSql_itemInsert = R"(
		insert into items (id, entity_id, container_id, item_level, count, position_base, position_secondary)
		values (:id, :entity_id, :container_id, :item_level, :count, :position_base, :position_secondary)
)";
}

class ItemsDataProviderDb::Private {
public:
	Private(ItemsDataProviderDb* paren) : q(paren) { }
	ItemsDataProviderDb* q;
	DatabasesService* databasesService;
};

ItemsDataProviderDb::ItemsDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

ItemsDataProviderDb::~ItemsDataProviderDb() = default;

std::shared_ptr<Item> ItemsDataProviderDb::item(const QUuid& id) const {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return std::shared_ptr<Item>();
	}

	auto reader = conn->executeQuery(QString("%1 and id = :item_id")
		.arg(kSql_itemSelect));
	reader->bindValue(":item_id", id
		.toString(QUuid::WithoutBraces)
		.toLower());

	if (!reader || !reader->next()) {
		qWarning() << "Can't find item:" << id;
		return std::shared_ptr<Item>();
	}

	DataProviders::Readers::ItemReader rd;
	return rd.read(*reader);
}

std::list<std::shared_ptr<Item>> ItemsDataProviderDb::itemsFromContainer(const QUuid& constainerId) const {
	std::list<std::shared_ptr<Item>> result;
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return result;
	}

	auto reader = conn->executeQuery(QString("%1 and container_id = :container_id")
		.arg(kSql_itemSelect));
	reader->bindValue(":container_id", constainerId
		.toString(QUuid::WithoutBraces)
		.toLower());

	while (reader->next()) {
		DataProviders::Readers::ItemReader rd;
		auto item = rd.read(*reader);
		result.push_back(item);
	}

	return result;
}

bool ItemsDataProviderDb::save(const Item& item) const {

	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return false;
	}

	auto insert = conn->prepare(kSql_itemInsert);
	insert->bindValue(":entity_id", item.entityId);
	insert->bindValue(":item_id", item.id
		.toString(QUuid::WithoutBraces)
		.toLower());
	insert->bindValue(":container_id", item.containerId
		.toString(QUuid::WithoutBraces)
		.toLower());
	insert->bindValue(":item_level", item.level);
	insert->bindValue(":count", item.count);
	insert->bindValue(":position_base", item.position.base);
	insert->bindValue(":position_secondary", item.position.secondary);

	if (!insert->exec()) {
		qWarning() << "Error saving item with entity" << item.entityId;
		return false;
	}

	return true;
}
