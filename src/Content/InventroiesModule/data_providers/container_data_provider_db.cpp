#include "Content/InventroiesModule/data_providers/container_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"
#include "Content/InventroiesModule/models/container.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

namespace {
static QString kSql_itemsSelets = R"(
		select i.id item_id, 
			ie."type",
			ie.sub_type,
			i.count,
			i.position_base,
			i.position_secondary,
			ie.width, 
			ie.height, 			
			ie.container_rows rows, 
			ie.container_cols cols,
			i.item_level level, 
			ie.rarity, 
			ie.icon_path 
		from items i 
		join item_entities ie on ie.id = i.entity_id 
			where i.ia_active = 1 and
				i.container_id = :container_id
)";
static QString kSql_containerSelect = R"(
		select c.name
		from containers c 
		join items i on i.id = c.item_id and i.ia_active = 1
		join item_entities ie on ie.id = i.entity_id 
			where ie."type" = 'equipment' and 
				ie.sub_type = 'root' and
				c.item_id = :container_id
)";
}

class ContainerDataProvider::Private {
public:
	Private(ContainerDataProvider* parent) : q(parent) { }
	ContainerDataProvider* q;
	
	DatabasesService* databasesService;

	void loadItems(Container& cont);
};

ContainerDataProvider::ContainerDataProvider(DatabasesService* databasesService)
: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

ContainerDataProvider::~ContainerDataProvider() = default;

void ContainerDataProvider::Private::loadItems(Container& cont) {
	auto conn = databasesService->connection("game");
	if (!conn) {
		return;
	}

	auto reader = conn->executeQuery(kSql_itemsSelets);
	if (!reader) {
		return;
	}

	reader->bindValue(":container_id", cont.id
		.toString(QUuid::WithoutBraces)
		.toLower());

	while (reader->next()) {
/*
		Inventory inv;
		inv.id = QUuid::fromString(reader->value("id").toString());
		inv.slot = static_cast<EquipmentSlotType>(reader->value("slot").toInt());
		inv.level = reader->value("level").toInt();
		inv.type = reader->value("type").toString();
		inv.subType = reader->value("sub_type").toString();
		inv.width = reader->value("width").toInt();
		inv.height = reader->value("height").toInt();
		inv.rarity = reader->value("rarity").toInt();
		inv.rows = reader->value("rows").toInt();
		inv.cols = reader->value("cols").toInt();
		inv.iconPath = reader->value("icon_path").toString();
		item.items.push_back(std::move(equipment));
*/
	}
}

std::shared_ptr<Container> ContainerDataProvider::container(const QUuid& id) {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return std::shared_ptr<Container>();
	}

	auto reader = conn->executeQuery(kSql_containerSelect);
	if (!reader) {
		return std::shared_ptr<Container>();
	}

	reader->bindValue(":container_id", id
		.toString(QUuid::WithoutBraces)
		.toLower());

	if (!reader->next()) {
		return std::shared_ptr<Container>();
	}

	auto item = std::make_shared<Container>();
	item->id = id;
	item->name = reader->value("name").toString();

	d->loadItems(*item);

	return item;
}

bool ContainerDataProvider::save(const QUuid& id, const Container& cont) {

	return true;
}
