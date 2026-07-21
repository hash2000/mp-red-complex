#include "Content/FetchApiModule/data_providers/fetch_queries_data_provider_db.h"
#include "Content/FetchApiModule/models/fetch_query.h"
#include "Content/FetchApiModule/models/fetch_tree.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"


namespace {
static QString kSql_querySelect = R"(
	SELECT "tree_id", "method", "request", "parameters", "headers", "body" FROM queries WHERE tree_id = :id;
)";
static QString kSql_queryDelete = R"(
	delete from queries where "tree_id" = :tree_id
)";
static QString kSql_queryUpdate = R"(
	update set
		"method" = :method,
		"request" = :request,
		"parameters" = :parameters,
		"headers" = :headers,
		"body" := body
		from queries
			where tree_id = :id
)";
}

class FetchQueriesDataProviderDb::Private {
public:
	Private(FetchQueriesDataProviderDb* paren) : q(paren) {}
	FetchQueriesDataProviderDb* q;

	DatabasesService* databasesService;
};

FetchQueriesDataProviderDb::FetchQueriesDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
}

FetchQueriesDataProviderDb::~FetchQueriesDataProviderDb() = default;

std::optional<FetchQuery> FetchQueriesDataProviderDb::query(int tree_id) {
	std::list<FetchTree> result;
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return std::nullopt;
	}

	auto reader = conn->executeQuery(kSql_querySelect);
	reader->bindValue(":id", tree_id);

	if (!reader || !reader->next()) {
		qCritical() << "FetchQueriesDataProviderDb::queries. Query error:" << conn->lastError();
		return std::nullopt;
	}

	FetchQuery query;
	query.method = reader->value("method").toString();
	query.request = reader->value("request").toString();

	auto parameters = reader->value("parameters");
	query.parameters = parameters.isNull() ? std::nullopt : std::optional<QString>(parameters.toString());

	auto headers = reader->value("headers");
	query.headers = headers.isNull() ? std::nullopt : std::optional<QString>(headers.toString());

	auto body = reader->value("body");
	query.body = body.isNull() ? std::nullopt : std::optional<QString>(body.toString());

	return query;
}

bool FetchQueriesDataProviderDb::deleteQuery(int treeId) {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return false;
	}
	auto deleteItem = conn->prepare(kSql_queryDelete);
	if (!deleteItem) {
		return false;
	}

	deleteItem->bindValue(":id", treeId);

	if (!deleteItem->exec()) {
		qWarning() << "Error delete query id" << treeId;
		return false;
	}

	return true;
}

bool FetchQueriesDataProviderDb::updateQuery(const FetchQuery& query) {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return false;
	}
	auto updateItem = conn->prepare(kSql_queryUpdate);
	if (!updateItem) {
		return false;
	}

	updateItem->bindValue(":method", query.method);
	updateItem->bindValue(":request", query.request);
	updateItem->bindValue(":parameters", query.parameters ? QVariant(query.parameters.value()) : QVariant());
	updateItem->bindValue(":headers", query.headers ? QVariant(query.headers.value()) : QVariant());
	updateItem->bindValue(":body", query.body ? QVariant(query.body.value()) : QVariant());

	if (!updateItem->exec()) {
		qWarning() << "Error update query id" << query.tree_id;
		return false;
	}

	return true;
}

