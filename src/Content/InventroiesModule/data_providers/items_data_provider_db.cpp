#include "Content/InventroiesModule/data_providers/items_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"
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

class ItemsDataProviderDb::Private {
public:
	Private(ItemsDataProviderDb* paren) : q(paren) { }
	ItemsDataProviderDb* q;

	DatabasesService* databasesService;

	std::unique_ptr<ItemEntity> readEntity(SQLiteReader& reader);
	ItemResourceType resourceType(const QString& type);
};

ItemsDataProviderDb::ItemsDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

ItemsDataProviderDb::~ItemsDataProviderDb() = default;

std::unique_ptr<ItemEntity> ItemsDataProviderDb::Private::readEntity(SQLiteReader& reader) {
	auto ent = std::make_unique<ItemEntity>();
	ent->id = reader.value("id").toString();
	ent->name = reader.value("name").toString();
	ent->description = reader.value("description").toString();
	ent->type = static_cast<ItemType>(reader.value("type").toInt());
	ent->subType = static_cast<ItemSubType>(reader.value("sub_type").toInt());
	ent->iconPath = reader.value("icon_path").toString();
	ent->width = reader.value("width").toInt();
	ent->height = reader.value("height").toInt();
	ent->rarity = static_cast<ItemRarityType>(reader.value("rarity").toInt());
	ent->maxStack = reader.value("max_stack").toInt();

	if (ent->type == ItemType::Resource) {
		const QString resourceIds = reader.value("resource_ids").toString();
		const QStringList ids = resourceIds.split(",");

		for (const auto& typeName : ids) {
			ent->resourceType.push_back(resourceType(typeName));
		}
	}
	else if (ent->type == ItemType::Container) {
		const QString permissions_for_all = reader.value("permissions_for_all").toString();
		const QStringList all = permissions_for_all.split(",");
		const QString permissions_for_any = reader.value("permissions_for_any").toString();
		const QStringList any = permissions_for_any.split(",");

		ent->container = ItemContainer();
		ent->container->rows = reader.value("container_rows").toInt();
		ent->container->cols = reader.value("container_cols").toInt();

		for (const auto& typeName : all) {
			ent->container->permissions.resources.all.push_back(resourceType(typeName));
		}

		for (const auto& typeName : any) {
			ent->container->permissions.resources.any.push_back(resourceType(typeName));
		}
	}

	return ent;
}

ItemResourceType ItemsDataProviderDb::Private::resourceType(const QString& type) {
	if (type == "ore") return ItemResourceType::Ore;
	else if (type == "chemical") return ItemResourceType::Chemical;
	else
		qWarning() << "Load item entity: Undefined resource type:" << type;
	return ItemResourceType::Undefined;
}

std::list<std::unique_ptr<ItemEntity>> ItemsDataProviderDb::entities() const {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return;
	}

	auto reader = conn->executeQuery(QString("%1\n%2")
		.arg(kSql_entitySelectorExpr)
		.arg(kSql_entityGroupByExpr));
	if (!reader) {
		return;
	}

	std::list<std::unique_ptr<ItemEntity>> result;

	while (reader->next()) {
		auto ent = d->readEntity(*reader);
		result.push_back(std::move(ent));
	}

	return std::move(result);
}

std::unique_ptr<ItemEntity> ItemsDataProviderDb::entity(const QString& id) const {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return;
	}
	auto reader = conn->executeQuery(QString("%1\nwhere e.id = :entity_id\n%2")
		.arg(kSql_entitySelectorExpr)
		.arg(kSql_entityGroupByExpr));

	reader->bindValue(":entity_id", id);

	if (!reader || !reader->next()) {
		qWarning() << "Can't find entity:" << id;
		return std::unique_ptr<ItemEntity>();
	}

	auto ent = d->readEntity(*reader);
	return std::move(ent);
}

std::unique_ptr<Item> ItemsDataProviderDb::item(const QUuid& id) const {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return;
	}

	auto reader = conn->executeQuery(R"(
		select entity_id, item_level
		from items
		where id = :item_id and ia_active = 1;
	)");

	reader->bindValue(":item_id", id
		.toString(QUuid::WithoutBraces)
		.toLower());

	if (!reader || !reader->next()) {
		qWarning() << "Can't find item:" << id;
		return std::unique_ptr<Item>();
	}

	auto item = std::make_unique<Item>();
	item->id = id;
	item->entityId = reader->value("entity_id").toString();
	item->level = reader->value("item_level").toInt();

	return std::move(item);
}

bool ItemsDataProviderDb::setItem(const QUuid& id, const Item& item) const {

	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return;
	}

	auto insert = conn->prepare(R"(
		insert into items (id, entity_id)
		values (:id, :entity_id)
	)");

	insert->bindValue(":entity_id", item.entityId);
	insert->bindValue(":item_id", id
		.toString(QUuid::WithoutBraces)
		.toLower());

	if (!insert->exec()) {
		qWarning() << "Error saving item with entity" << item.entityId;
		return false;
	}

	return true;
}
