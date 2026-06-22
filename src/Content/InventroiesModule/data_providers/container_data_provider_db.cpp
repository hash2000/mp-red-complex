#include "Content/InventroiesModule/data_providers/container_data_provider_db.h"
#include "Content/InventroiesModule/data_providers/migrations/inventories_migrations.h"
#include "Content/InventroiesModule/models/container.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

namespace {
static QString kSql_containersSelect = R"(
		select
			c.item_id container_id,
			c.name
		from containers c
		where	c.item_id = :container_id
)";
}

class ContainerDataProvider::Private {
public:
	Private(ContainerDataProvider* parent) : q(parent) { }
	ContainerDataProvider* q;
	
	DatabasesService* databasesService;
};

ContainerDataProvider::ContainerDataProvider(DatabasesService* databasesService)
: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	auto migrator = d->databasesService->migrationManager("game");
	InventoriesMigrations::build(migrator);
}

ContainerDataProvider::~ContainerDataProvider() = default;

std::list<std::shared_ptr<Container>> ContainerDataProvider::containers(const QUuid& id) {
	std::list<std::shared_ptr<Container>> result;
	auto conn = d->databasesService->connection("game");
	if (!conn) {
		return result;
	}

	auto reader = conn->executeQuery(kSql_containersSelect);
	if (!reader) {
		return result;
	}

	while (reader->next()) {
		auto container = std::shared_ptr<Container>();
		container->id = QUuid::fromString(reader->value("container_id").toString());
		container->name = reader->value("name").toString();
		result.push_back(container);
	}

	return result;
}

bool ContainerDataProvider::save(const QUuid& id, const Container& cont) {

	return true;
}
