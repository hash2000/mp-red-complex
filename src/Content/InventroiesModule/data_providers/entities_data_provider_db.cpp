#include "Content/InventroiesModule/data_providers/entities_data_provider_db.h"
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

class EntitiesDataProviderDb::Private {
public:
	Private(EntitiesDataProviderDb* paren) : q(paren) {}
	EntitiesDataProviderDb* q;

	DatabasesService* databasesService;

	std::shared_ptr<ItemEntity> readEntity(SQLiteReader& reader);
	ItemResourceType resourceType(const QString& type);
	ItemType itemType(const QString& type);
	ItemSubType itemSubType(const QString& type);
};

EntitiesDataProviderDb::EntitiesDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

EntitiesDataProviderDb::~EntitiesDataProviderDb() = default;

std::shared_ptr<ItemEntity> EntitiesDataProviderDb::Private::readEntity(SQLiteReader& reader) {
	auto ent = std::make_shared<ItemEntity>();
	ent->id = QUuid::fromString(reader.value("id").toString());
	ent->name = reader.value("name").toString();
	ent->type = itemType(reader.value("type").toString());
	ent->subType = itemSubType(reader.value("sub_type").toString());
	ent->description = reader.value("description").toString();
	ent->iconPath = reader.value("icon_path").toString();
	ent->width = reader.value("width").toInt();
	ent->height = reader.value("height").toInt();
	ent->rarity = static_cast<ItemRarityType>(reader.value("rarity").toInt());
	ent->maxStack = reader.value("max_stack").toInt();

	if (ent->type == ItemType::Resource) {
		const QString resourceIds = reader.value("resource_ids").toString();
		const QStringList ids = resourceIds.split(",", Qt::SkipEmptyParts);

		for (const auto& typeName : ids) {
			ent->resourceType.push_back(resourceType(typeName));
		}
	}
	else if (ent->type == ItemType::Container) {
		const QString permissions_for_all = reader.value("permissions_for_all").toString();
		const QStringList all = permissions_for_all.split(",", Qt::SkipEmptyParts);
		const QString permissions_for_any = reader.value("permissions_for_any").toString();
		const QStringList any = permissions_for_any.split(",", Qt::SkipEmptyParts);

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

ItemResourceType EntitiesDataProviderDb::Private::resourceType(const QString& type) {
	if (type == "ore") return ItemResourceType::Ore;
	else if (type == "chemical") return ItemResourceType::Chemical;
	else
		qWarning() << "Load item entity : Undefined resource type:" << type;
	return ItemResourceType::Undefined;
}

ItemType EntitiesDataProviderDb::Private::itemType(const QString& type) {
	if (type == "resource") return ItemType::Resource;
	else if (type == "equipment") return ItemType::Equipment;
	else if (type == "component") return ItemType::Component;
	else if (type == "container") return ItemType::Container;
	else if (type == "recipe") return ItemType::Recipe;
	else
		qWarning() << "Load item entity: Undefined type:" << type;
	return ItemType::Undefined;
}

ItemSubType EntitiesDataProviderDb::Private::itemSubType(const QString& type) {
	if (type == "head") return ItemSubType::Head;
	if (type == "body") return ItemSubType::Body;
	if (type == "weapon") return ItemSubType::Weapon;
	if (type == "shield") return ItemSubType::Shield;
	if (type == "gloves") return ItemSubType::Gloves;
	if (type == "boots") return ItemSubType::Boots;
	if (type == "ring") return ItemSubType::Ring;
	if (type == "amulet") return ItemSubType::Amulet;
	if (type == "resource") return ItemSubType::Resource;
	if (type == "consumable") return ItemSubType::Consumable;
	if (type == "backpack") return ItemSubType::Backpack;
	if (type == "root") return ItemSubType::Root;
	else
		qWarning() << "Load item entity: Undefined sub type:" << type;
	return ItemSubType::Undefined;
}

std::list<std::shared_ptr<ItemEntity>> EntitiesDataProviderDb::entities() const {
	std::list<std::shared_ptr<ItemEntity>> result;
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return std::move(result);
	}

	auto reader = conn->executeQuery(QString("%1\n%2")
		.arg(kSql_entitySelectorExpr)
		.arg(kSql_entityGroupByExpr));

	if (!reader) {
		return std::move(result);
	}

	while (reader->next()) {
		auto ent = d->readEntity(*reader);
		result.push_back(std::move(ent));
	}

	return std::move(result);
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

	auto ent = d->readEntity(*reader);
	return std::move(ent);
}
