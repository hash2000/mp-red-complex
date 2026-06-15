#include "Content/InventroiesModule/data_providers/inventories_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"

#include <QDir>

class InventoriesDataProviderDb::Private {
public:
	Private(InventoriesDataProviderDb* parent) : q(parent) { }
	InventoriesDataProviderDb* q;
	DatabasesService* databasesService;
};


InventoriesDataProviderDb::InventoriesDataProviderDb(DatabasesService* databasesService)
: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

InventoriesDataProviderDb::~InventoriesDataProviderDb() = default;


void InventoriesDataProviderDb::loadInventories(std::list<QUuid>& inventories) const {
	//const QString basePath = d->resources->Variables.get("Resources.Path", "").toString();
	//const QString path = QDir(basePath).filePath("data/inventory");

	//QDir dir(path);
	//if (!dir.exists()) {
	//	qWarning() << "InventoriesDataProviderDb: path does not exist:" << path;
	//	return;
	//}

	//const auto entries = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
	//for (const auto& entry : entries) {
	//	const QString fileName = entry.baseName();
	//	const QUuid id(fileName);
	//	if (!id.isNull()) {
	//		inventories.push_back(id);
	//	}
	//}
}

void InventoriesDataProviderDb::loadEquipments(std::list<QUuid>& equipments) const {
	//const QString basePath = d->resources->Variables.get("Resources.Path", "").toString();
	//const QString path = QDir(basePath).filePath("data/equipment");

	//QDir dir(path);
	//if (!dir.exists()) {
	//	qWarning() << "InventoriesDataProviderDb: path does not exist:" << path;
	//	return;
	//}

	//const auto entries = dir.entryInfoList(QStringList() << "*.json", QDir::Files);
	//for (const auto& entry : entries) {
	//	const QString fileName = entry.baseName();
	//	const QUuid id(fileName);
	//	if (!id.isNull()) {
	//		equipments.push_back(id);
	//	}
	//}
}
