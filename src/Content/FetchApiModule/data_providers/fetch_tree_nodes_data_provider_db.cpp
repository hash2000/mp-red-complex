#include "Content/FetchApiModule/data_providers/fetch_tree_nodes_data_provider_db.h"
#include "Content/FetchApiModule/models/fetch_tree.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"
#include <QJsonObject>

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
update queries set
	parent_id = :parent_id,
	name = :name,
	data = :data
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

	static void writeNodeProperties(std::unique_ptr<SQLiteReader>& reader, const QVariantMap& properties);
	static void readPropertiesToNode(const std::unique_ptr<SQLiteReader>& reader, LazyTreeNodePtr node);
	static LazyTreeNodeList readNodesList(const std::unique_ptr<SQLiteReader>& reader);
};

FetchTreeNodesDataProviderDb::FetchTreeNodesDataProviderDb(DatabasesService* databasesService)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
}

FetchTreeNodesDataProviderDb::~FetchTreeNodesDataProviderDb() = default;

void FetchTreeNodesDataProviderDb::Private::readPropertiesToNode(const std::unique_ptr<SQLiteReader>& reader, LazyTreeNodePtr node) {
	const auto value = reader->value("data").toString();

	QJsonParseError error;
	QJsonDocument doc = QJsonDocument::fromJson(value.toUtf8(), &error);

	if (error.error != QJsonParseError::NoError) {
		qWarning() << "JSON parse error:" << error.errorString();
		return;
	}

	if (!doc.isObject()) {
		qWarning() << "Expected JSON object, but got something else";
		return;
	}

	const auto jsonObj = doc.object();
	const auto map = jsonObj.toVariantMap();

	for (const auto& key : map.keys()) {
		const auto& value = map.value(key);
		node->setProperty(key, value);
	}
}

LazyTreeNodeList FetchTreeNodesDataProviderDb::Private::readNodesList(const std::unique_ptr<SQLiteReader>& reader) {
	LazyTreeNodeList result;

	while (reader->next()) {
		const auto item = std::make_shared<FetchTree>();
		readPropertiesToNode(reader, item);

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

LazyTreeNodeList FetchTreeNodesDataProviderDb::treeNodes(const QVariant& parentId) const {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return LazyTreeNodeList();
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
		return LazyTreeNodeList();
	}

	return d->readNodesList(reader);
}

LazyTreeNodeList FetchTreeNodesDataProviderDb::searchTreeNodes(const QString& text) const {
	auto conn = d->databasesService->connection("fetch_api");
	if (!conn) {
		return LazyTreeNodeList();
	}

	auto reader = conn->executeQuery(kSql_treeNodesSerach);
	if (!reader) {
		qCritical() << "FetchTreeNodesDataProviderDb::tsearchTeeNodes. Query error:" << conn->lastError();
		return LazyTreeNodeList();
	}

	reader->bindValue(":search_text", text);

	return d->readNodesList(reader);
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

void FetchTreeNodesDataProviderDb::Private::writeNodeProperties(std::unique_ptr<SQLiteReader>& reader, const QVariantMap& properties) {
	if (properties.count() > 0) {
		const auto json = QJsonObject::fromVariantMap(properties);
		QJsonDocument docCompact(json);
		const auto compactBytes = docCompact.toJson(QJsonDocument::Compact);
		const auto str = QString::fromUtf8(compactBytes);
		reader->bindValue(":data", str);
	}
	else {
		reader->bindValue(":data", "{}");
	}

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

	d->writeNodeProperties(updateItem, node->properties());
	updateItem->bindValue(":parent_id", node->parentId());
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

	insertItem->bindValue(":parent_id", node->parentId());
	insertItem->bindValue(":name", node->name());

	d->writeNodeProperties(insertItem, node->properties());

	if (!insertItem->exec()) {
		qWarning() << "Error insert path id" << node->id();
		return false;
	}

	node->setId(QVariant::fromValue((int)insertItem->lastInsert()));
	return true;
}

