#include "Content/FetchApiModule/data_providers/fetch_tree_nodes_data_provider_db.h"
#include "Content/FetchApiModule/models/fetch_tree.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"


namespace {
static QString kSql_treeNodesSelect = R"(
	select
		tree_id id,
		parent_id,
		name,
		case
				when exists (
					select 1
					from queries_tree as child
					where child.parent_id = t.tree_id
				)
				then 1
				else 0
			end as has_children
	from queries_tree as t;
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
static QString kSql_treeNodeInsert = R"(
	insert into queries_tree (parent_id, name) values(:parent_id, :name);
)";
static QString kSql_treeNodesSerach = R"(
	with cte as (
		select qt.tree_id id, qt.parent_id, qt.name, 1 IsFound, 0 IsExpanded
		from queries_tree qt
			where :search_text is not null and qt.name like concat('%', :search_text, '%')
		
		union all
	
		select p.tree_id id, p.parent_id, p.name, 0 IsFound, 1 IsExpanded
		from queries_tree p
			join cte c on c.parent_id = p.tree_id 
	)
	select * from cte
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

LazyTreeNodeList FetchTreeNodesDataProviderDb::treeNodes(std::optional<int> parentId) const {
	LazyTreeNodeList result;
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
		const auto item = std::make_shared<FetchTree>();
		item->setId(reader->value("id").toInt());
		item->setName(reader->value("name").toString());
		auto parentId = reader->value("parent_id");
		if (!parentId.isNull()) {
			item->setParentId(parentId.toInt());
		}

		if (reader->value("has_children").toInt() > 0) {
			item->setChildren(LazyTreeNodeList());
		}

		result.push_back(item);
	}

	return result;
}

LazyTreeNodeList FetchTreeNodesDataProviderDb::searchTreeNodes(const QString& text) const {

	LazyTreeNodeList result;
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return result;
	}

	auto reader = conn->executeQuery(kSql_treeNodesSerach);
	if (!reader) {
		qCritical() << "FetchTreeNodesDataProviderDb::tsearchTeeNodes. Query error:" << conn->lastError();
		return result;
	}

	reader->bindValue(":search_text", text);

	while (reader->next()) {
		const auto item = std::make_shared<FetchTree>();
		item->setId(reader->value("id").toInt());
		item->setName(reader->value("name").toString());
		auto parentId = reader->value("parent_id");
		if (!parentId.isNull()) {
			item->setParentId(parentId.toInt());
		}

		if (reader->value("has_children").toInt() > 0) {
			item->setChildren(LazyTreeNodeList());
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

bool FetchTreeNodesDataProviderDb::updateTreeNode(const LazyTreeNodePtr& node) {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return false;
	}
	auto updateItem = conn->prepare(kSql_treeNodesUpdate);
	if (!updateItem) {
		return false;
	}

	updateItem->bindValue(":parent_id", node->parentId() ? QVariant(node->parentId().value()) : QVariant());
	updateItem->bindValue(":name", node->name());
	updateItem->bindValue(":id", node->id());

	if (!updateItem->exec()) {
		qWarning() << "Error update path id" << node->id();
		return false;
	}

	return true;
}

bool FetchTreeNodesDataProviderDb::addNode(const LazyTreeNodePtr& node) {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return false;
	}
	auto insertItem = conn->prepare(kSql_treeNodeInsert);
	if (!insertItem) {
		return false;
	}

	insertItem->bindValue(":parent_id", node->parentId() ? QVariant(node->parentId().value()) : QVariant());
	insertItem->bindValue(":name", node->name());

	if (!insertItem->exec()) {
		qWarning() << "Error insert path id" << node->id();
		return false;
	}

	node->setId(insertItem->lastInsert());
	return true;
}

