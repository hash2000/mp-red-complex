#include "Content/InventroiesModule/data_providers/items_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

namespace {
static QString kSql_itemSelect = R"(
		select entity_id, item_level
		from items
		where id = :item_id and ia_active = 1;
)";
static QString kSql_itemInsert = R"(
		insert into items (id, entity_id)
		values (:id, :entity_id)
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

	auto reader = conn->executeQuery(kSql_itemSelect);
	reader->bindValue(":item_id", id
		.toString(QUuid::WithoutBraces)
		.toLower());

	if (!reader || !reader->next()) {
		qWarning() << "Can't find item:" << id;
		return std::shared_ptr<Item>();
	}

	auto item = std::make_shared<Item>();
	item->id = id;
	item->entityId = QUuid::fromString(reader->value("entity_id").toString());
	item->level = reader->value("item_level").toInt();

	return std::move(item);
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

	if (!insert->exec()) {
		qWarning() << "Error saving item with entity" << item.entityId;
		return false;
	}

	return true;
}
