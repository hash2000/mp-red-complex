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
static QString kSql_containersSelect = R"(
		select i.id, i.entity_id, i.container_id, i.item_level level, i.count, i.position_base, i.position_secondary
		from items i
		join containers c on c.item_id = i.id 
		join item_entities ie on ie.id = i.entity_id 
		where ia_active = 1 
)";
static QString kSql_itemInsert = R"(
		insert into items (id, entity_id, container_id, item_level, count, position_base, position_secondary)
		values (:id, :entity_id, :container_id, :item_level, :count, :position_base, :position_secondary)
)";
static QString kSql_containerInsert = R"(
		insert into containers (item_id, name)
		values (:item_id, :name)
)";
static QString kSql_changeItemContaiter = R"(
		update items 
		set container_id = :container_id
		where id = :item_id
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

	Inventory::Readers::ItemReader rd;
	return rd.read(*reader);
}

std::list<std::shared_ptr<Item>> ItemsDataProviderDb::containers() const {
	std::list<std::shared_ptr<Item>> result;
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return result;
	}

	auto reader = conn->executeQuery(QString("%1 and ie.type = 'container'")
		.arg(kSql_containersSelect));

	while (reader->next()) {
		Inventory::Readers::ItemReader rd;
		auto item = rd.read(*reader);
		result.push_back(item);
	}

	return result;
}

std::list<std::shared_ptr<Item>> ItemsDataProviderDb::equipments() const {
	std::list<std::shared_ptr<Item>> result;
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return result;
	}

	auto reader = conn->executeQuery(QString("%1 and ie.type = 'equipment'")
		.arg(kSql_containersSelect));

	while (reader->next()) {
		Inventory::Readers::ItemReader rd;
		auto item = rd.read(*reader);
		result.push_back(item);
	}

	return result;
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
		Inventory::Readers::ItemReader rd;
		auto item = rd.read(*reader);
		result.push_back(item);
	}

	return result;
}

bool ItemsDataProviderDb::changeContainer(const Item& item, const QUuid& constainerId) const {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return false;
	}

	auto update = conn->prepare(kSql_changeItemContaiter);
	if (!update) {
		return false;
	}

	update->bindValue(":container_id", constainerId
		.toString(QUuid::WithoutBraces)
		.toLower());
	update->bindValue(":item_id", item.id
		.toString(QUuid::WithoutBraces)
		.toLower());

	return update->exec();
}

bool ItemsDataProviderDb::save(const Item& item) const {

	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return false;
	}

	const auto isContainer =
		item.entity->subType == ItemSubType::Root && (
		item.entity->type == ItemType::Container ||
		item.entity->type == ItemType::Equipment);

	conn->beginTransaction();

	auto insertItem = conn->prepare(kSql_itemInsert);
	if (!insertItem) {
		return false;
	}

	insertItem->bindValue(":entity_id", item.entityId);
	insertItem->bindValue(":item_id", item.id
		.toString(QUuid::WithoutBraces)
		.toLower());
	insertItem->bindValue(":container_id", item.containerId
		.toString(QUuid::WithoutBraces)
		.toLower());
	insertItem->bindValue(":item_level", item.level);
	insertItem->bindValue(":count", item.count);
	insertItem->bindValue(":position_base", item.position.base);
	insertItem->bindValue(":position_secondary", item.position.secondary);

	if (!insertItem->exec()) {
		qWarning() << "Error saving item with entity" << item.entityId << "item" << item.id;
		conn->rollback();
		return false;
	}

	if (!isContainer) {
		return true;
	}

	auto insertContainer = conn->prepare(kSql_containerInsert);
	if (!insertContainer) {
		return false;
	}

	insertContainer->bindValue(":item_id", item.id
		.toString(QUuid::WithoutBraces)
		.toLower());
	insertContainer->bindValue("name", item.entity->name);

	if (!insertContainer->exec()) {
		qWarning() << "Error saving item as container with entity" << item.entityId << "item" << item.id;
		conn->rollback();
		return false;
	}

	if (!conn->commit()) {
		qWarning() << "Error commit saving item as container with entity" << item.entityId << "item" << item.id;
		return false;
	}

	return true;
}
