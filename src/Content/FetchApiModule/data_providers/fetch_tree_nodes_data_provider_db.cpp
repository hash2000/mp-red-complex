#include "Content/FetchApiModule/data_providers/fetch_tree_nodes_data_provider_db.h"
#include "Content/FetchApiModule/models/fetch_tree.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"


namespace {
static QString kSql_treeNodesSelect = R"(
	select
		id,
		parent_id,
		name,
		data,
		case
				when exists (
					select 1
					from queries as child
					where child.parent_id = t.id
				)
				then 1
				else 0
			end as has_children,
		0 expanded
	from queries as t
)";
static QString kSql_treeNodesDelete = R"(
	delete from queries where id = :id
)";
static QString kSql_treeNodesUpdate = R"(
update set
	parent_id = :parent_id,
	name = :name,
	data = :data
	from queries
		where id = :id
)";
static QString kSql_treeNodeInsert = R"(
	insert into queries (parent_id, name, data) values(:parent_id, :name, :data);
)";
static QString kSql_treeNodesSerach = R"(
with recursive
found as (
    select
        qt.id as id,
        qt.parent_id,
        qt.name,
        case
            when exists (
                select 1
                from queries as child
                where child.parent_id = qt.id
            )
            then 1
            else 0
        end as has_children
    from queries qt
    where qt.name like '%' || :search_text || '%'
),
raw as (
    -- найденные узлы: изначально считаем их закрытыми
    select
        id,
        parent_id,
        name,
        has_children,
        0 as expanded
    from found
    union all
    -- идем вверх к корню: эти узлы уже должны быть раскрыты
    select
        p.id,
        p.parent_id,
        p.name,
        1 as has_children,
        1 as expanded
    from queries p
    join raw c on c.parent_id = p.id
),
cte as (
    select
        id,
        parent_id,
        name,
        max(has_children) as has_children,
        max(expanded) as expanded
    from raw
    group by
        id,
        parent_id,
        name
)
select
	q.id,
	q.parent_id,
	q.name,
	q.data, 
	cte.has_children,
	cte.expanded
from cte
join queries q on q.id = cte.id
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

LazyTreeNodeList FetchTreeNodesDataProviderDb::treeNodes(const QVariant& parentId) const {
	LazyTreeNodeList result;
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return result;
	}

	QString where;
	if (parentId.isNull()) where = "is null";
	else where = QString("= %1").arg(parentId.toInt());
	auto query = QString("%1\nwhere parent_id %2")
		.arg(kSql_treeNodesSelect)
		.arg(where);

	auto reader = conn->executeQuery(query);
	if (!reader) {
		qCritical() << "FetchTreeNodesDataProviderDb::treeNodes. Query error:" << conn->lastError();
		return result;
	}

	while (reader->next()) {
		const auto item = std::make_shared<FetchTree>();
		item->setId(reader->value("id"));
		item->setName(reader->value("name").toString());
		item->setExpanded(reader->value("expanded").toInt() != 0);
		auto parentId = reader->value("parent_id");
		if (!parentId.isNull()) {
			item->setParentId(parentId);
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
		item->setId(reader->value("id"));
		item->setName(reader->value("name").toString());
		item->setExpanded(reader->value("expanded").toInt() != 0);
		auto parentId = reader->value("parent_id");
		if (!parentId.isNull()) {
			item->setParentId(parentId);
		}

		if (reader->value("has_children").toInt() != 0) {
			item->setChildren(LazyTreeNodeList());
		}

		result.push_back(item);
	}

	return result;
}

bool FetchTreeNodesDataProviderDb::deleteNode(const QVariant& id) {
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

bool FetchTreeNodesDataProviderDb::updateNode(const LazyTreeNodePtr& node) {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return false;
	}
	auto updateItem = conn->prepare(kSql_treeNodesUpdate);
	if (!updateItem) {
		return false;
	}

	updateItem->bindValue(":parent_id", node->parentId());
	updateItem->bindValue(":name", node->name());
	updateItem->bindValue(":id", node->id());
	updateItem->bindValue(":data", "{}");

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

	insertItem->bindValue(":parent_id", node->parentId());
	insertItem->bindValue(":name", node->name());
	insertItem->bindValue(":data", "{}");

	if (!insertItem->exec()) {
		qWarning() << "Error insert path id" << node->id();
		return false;
	}

	node->setId(QVariant::fromValue((int)insertItem->lastInsert()));
	return true;
}

