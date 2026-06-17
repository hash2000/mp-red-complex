#include "Content/InventroiesModule/data_providers/equipment_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"
#include "Content/InventroiesModule/models/equipment.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

namespace {
static QString kSql_itemsSelets = R"(
		select ci.item_id, 
			ci.position_base slot, 
			i.item_level level, 
			ie."type",
			ie.sub_type, 
			ie.width, 
			ie.height, 
			ie.rarity, 
			ie.container_rows rows, 
			ie.container_cols cols, 
			ie.icon_path 
		from container_items ci
		join items i on i.id = ci.item_id and i.ia_active = 1
		join item_entities ie on ie.id = i.entity_id 
			where ie."type" in ('equipment', 'container') and
				ie.sub_type != 'system' and
				ci.container_id = :container_id
)";
static QString kSql_equipmentSelect = R"(
		select c.name
		from containers c 
		join items i on i.id = c.item_id and i.ia_active = 1
		join item_entities ie on ie.id = i.entity_id 
			where ie."type" = 'equipment' and 
				ie.sub_type = 'system' and
				c.item_id = :container_id
)";
}

class EquipmentDataProviderDb::Private {
public:
	Private(EquipmentDataProviderDb* parent)
		: q(parent) {
	}

	EquipmentDataProviderDb* q;
	DatabasesService* databasesService;

	void loadItems(Equipment& item);
};

EquipmentDataProviderDb::EquipmentDataProviderDb(DatabasesService* databasesService)
: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

EquipmentDataProviderDb::~EquipmentDataProviderDb() = default;

void EquipmentDataProviderDb::Private::loadItems(Equipment& item) {
	auto conn = databasesService->connection("game");
	if (!conn) {
		return;
	}

	auto reader = conn->executeQuery(kSql_itemsSelets);
	if (!reader) {
		return;
	}
	reader->bindValue(":container_id", item.id
		.toString(QUuid::WithoutBraces)
		.toLower());

	while (reader->next()) {
		EquipmentItem equipment;
		equipment.id = QUuid::fromString(reader->value("id").toString());
		equipment.slot = static_cast<EquipmentSlotType>(reader->value("slot").toInt());
		equipment.level = reader->value("level").toInt();
		equipment.type = reader->value("type").toString();
		equipment.subType = reader->value("sub_type").toString();
		equipment.width = reader->value("width").toInt();
		equipment.height = reader->value("height").toInt();
		equipment.rarity = reader->value("rarity").toInt();
		equipment.rows = reader->value("rows").toInt();
		equipment.cols = reader->value("cols").toInt();
		equipment.iconPath = reader->value("icon_path").toString();
		item.items.push_back(std::move(equipment));
	}
}

std::shared_ptr<Equipment> EquipmentDataProviderDb::equipment(const QUuid& id) {
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return std::shared_ptr<Equipment>();
	}

	auto reader = conn->executeQuery(kSql_equipmentSelect);
	if (!reader) {
		return std::shared_ptr<Equipment>();
	}

	reader->bindValue(":container_id", id
		.toString(QUuid::WithoutBraces)
		.toLower());

	if (!reader->next()) {
		return std::shared_ptr<Equipment>();
	}

	auto item = std::make_shared<Equipment>();
	item->id = id;
	item->name = reader->value("name").toString();

	d->loadItems(*item);

	return item;
}

bool EquipmentDataProviderDb::save(const QUuid& id, const Equipment& equipment) const {

	return true;
}
