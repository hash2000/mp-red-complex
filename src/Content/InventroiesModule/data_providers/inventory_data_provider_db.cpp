#include "Content/InventroiesModule/data_providers/inventory_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/InventroiesModule/models/inventory_item.h"


class InventoryDataProviderDb::Private {
public:
	Private(InventoryDataProviderDb* parent)
	: q(parent) {
	}

	InventoryDataProviderDb* q;
	DatabasesService* databasesService;
};


InventoryDataProviderDb::InventoryDataProviderDb(DatabasesService* databasesService)
: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

InventoryDataProviderDb::~InventoryDataProviderDb() = default;

std::optional<Inventory> InventoryDataProviderDb::loadInventory(const QUuid& id) const {
	//const auto path = QString("inventory/%1.json")
	//	.arg(id.toString(QUuid::StringFormat::WithoutBraces).toLower());

	//QJsonObject json;
	//Format::Json::DataReader reader(d->resources, "data", path);
	//if (!reader.read(json)) {
	//	return false;
	//}

	//inventory.id = id;
	//inventory.rows = json["rows"].toInt();
	//inventory.cols = json["cols"].toInt();
	//inventory.name = json["name"].toString();
	//const auto itemsArr = json["items"].toArray();

	//for (const QJsonValue& itemObj : itemsArr) {
	//	InventoryItem item;
	//	item.id = QUuid::fromString(itemObj["id"].toString());
	//	item.count = itemObj["count"].toInt();
	//	item.x = itemObj["x"].toInt();
	//	item.y = itemObj["y"].toInt();
	//	inventory.items.push_back(item);
	//}

	return std::nullopt;
}

bool InventoryDataProviderDb::saveInventory(const QUuid& id, const Inventory& inventory) const {


	return true;
}
