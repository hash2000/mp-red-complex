#include "Content/FetchApiModule/data_providers/fetch_tree_nodes_data_provider_db.h"
#include "Content/FetchApiModule/models/fetch_tree.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"


namespace {
static QString kSql_treeNodesSelect = R"(
SELECT
	id,
	parent_id,
	name,
	CASE
			WHEN EXISTS (
				SELECT 1
				FROM queries_tree AS child
				WHERE child.parent_id = t.id
			)
			THEN 1
			ELSE 0
		END AS has_children
FROM queries_tree AS t;
)";
static QString kSql_treeNodesDelete = R"(
	delete from queries_tree where id = :id
)";
static QString kSql_treeNodesUpdate = R"(
	update set
		parent_id = :parent_id
		name = :name
		from queries_tree
			where id = :id
)";
}

class FetchTreeNodesDataProviderDb::Private {
public:
	Private(FetchTreeNodesDataProviderDb* paren) : q(paren) {}
	FetchTreeNodesDataProviderDb* q;

	DatabasesService* databasesService;
};

FetchTreeNodesDataProviderDb::FetchTreeNodesDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
}

FetchTreeNodesDataProviderDb::~FetchTreeNodesDataProviderDb() = default;

std::list<FetchTree> FetchTreeNodesDataProviderDb::treeNodes(std::optional<int> parentId) {
	std::list<FetchTree> result;
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return result;
	}

	QString where;
	if (!parentId) where = "is null";
	else where = QString("= %1").arg(parentId.value());

	auto reader = conn->executeQuery(QString("%1\nwhere t.parent_id %2")
		.arg(kSql_treeNodesSelect)
		.arg(where));

	if (!reader) {
		qCritical() << "FetchTreeNodesDataProviderDb::treeNodes. Query error:" << conn->lastError();
		return result;
	}

	while (reader->next()) {
		FetchTree item;
		item.id = reader->value("id").toInt();
		item.name = reader->value("name").toString();
		auto parentId = reader->value("parent_id");
		if (!parentId.isNull()) {
			item.parentId = parentId.toInt();
		}

		if (reader->value("has_children").toInt() > 0) {
			// добавляю пустой список, для ленивой загрузки дочерних узлов
			// если std::nullopt, это охначает - дочерних узлов нет
			item.children = std::list<FetchTree>();
		}

		result.push_back(item);
	}

	return result;
}

bool FetchTreeNodesDataProviderDb::deleteTreeNode(int id) {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return false;
	}
	auto deleteItem = conn->prepare(kSql_treeNodesDelete);
	if (!deleteItem) {
		return false;
	}

	deleteItem->bindValue(":id", id);

	if (!deleteItem->exec()) {
		qWarning() << "Error delete path id" << id;
		return false;
	}

	return true;
}

bool FetchTreeNodesDataProviderDb::updateTreeNode(const FetchTree& node) {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return false;
	}
	auto updateItem = conn->prepare(kSql_treeNodesUpdate);
	if (!updateItem) {
		return false;
	}

	updateItem->bindValue(":parent_id", node.parentId ? QVariant(node.parentId.value()) : QVariant());
	updateItem->bindValue(":name", node.name);
	updateItem->bindValue(":id", node.id);

	if (!updateItem->exec()) {
		qWarning() << "Error update path id" << node.id;
		return false;
	}

	return true;
}

