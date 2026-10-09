#include "Libs/BaseWidgets/tree_view/lazy_load/data_providers/lazy_tree_data_provider_db.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_builder.h"
#include "libs/Resources/db/sqlite/sqlite_connection.h"
#include "libs/Resources/db/sqlite/sqlite_reader.h"

#include "Content/DatabaseModule/services/databases_service.h"

#include <QJsonObject>

namespace {
static const QString kSql_treeNodesSelectTemplate = R"(
	select
		id,
		parent_id,
		name,
		data,
		case
				when exists (
					select 1
					from {{table}} as child
					where child.parent_id = t.id
				)
				then 1
				else 0
			end as has_children,
		0 expanded
	from {{table}} as t
)";
static const QString kSql_treeNodesDeleteTemplate = R"(
	delete from {{table}} where id = :id
)";
static const QString kSql_treeNodesUpdateTemplate = R"(
update {{table}} set
	parent_id = :parent_id,
	name = :name,
	data = :data
		where id = :id
)";
static const QString kSql_treeNodeInsertTemplate = R"(
	insert into {{table}} (parent_id, name, data) values(:parent_id, :name, :data);
)";
static const QString kSql_treeNodesSerachTemplate = R"(
with recursive
found as (
    select
        qt.id as id,
        qt.parent_id,
        qt.name,
        case
            when exists (
                select 1
                from {{table}} as child
                where child.parent_id = qt.id
            )
            then 1
            else 0
        end as has_children
    from {{table}} qt
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
    from {{table}} p
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
join {{table}} q on q.id = cte.id
)";
}

class LazyNodesDataProviderDb::Private {
public:
	Private(LazyNodesDataProviderDb* parent) : q(parent) {}
	LazyNodesDataProviderDb* q;

	DatabasesService* databasesService;
	QString databaseName;
	QString sqlSelect;
	QString sqlInsert;
	QString sqlDelete;
	QString sqlUpdate;
	QString sqlSearch;

	static QString buildSql(const QString& tpl, const QString& tableName);
	static void writeNodeProperties(std::unique_ptr<SQLiteReader>& reader, const QVariantMap& properties);
	static void readPropertiesToNode(const std::unique_ptr<SQLiteReader>& reader, LazyTreeNodePtr node);
	LazyTreeNodeList readNodesList(const std::unique_ptr<SQLiteReader>& reader);
};

LazyNodesDataProviderDb::LazyNodesDataProviderDb(DatabasesService* databasesService, const QString& databaseName, const QString& tableName)
	: d(std::make_unique<Private>(this)) {
	d->databasesService = databasesService;
	d->databaseName = databaseName;
	d->sqlSelect = Private::buildSql(kSql_treeNodesSelectTemplate, tableName);
	d->sqlInsert = Private::buildSql(kSql_treeNodeInsertTemplate, tableName);
	d->sqlDelete = Private::buildSql(kSql_treeNodesDeleteTemplate, tableName);
	d->sqlUpdate = Private::buildSql(kSql_treeNodesUpdateTemplate, tableName);
	d->sqlSearch = Private::buildSql(kSql_treeNodesSerachTemplate, tableName);
}

LazyNodesDataProviderDb::~LazyNodesDataProviderDb() = default;

QString LazyNodesDataProviderDb::Private::buildSql(const QString& tpl, const QString& tableName) {
	QString result = tpl;
	return result.replace("{{table}}", tableName);
}

void LazyNodesDataProviderDb::Private::readPropertiesToNode(const std::unique_ptr<SQLiteReader>& reader, LazyTreeNodePtr node) {
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

LazyTreeNodeList LazyNodesDataProviderDb::Private::readNodesList(const std::unique_ptr<SQLiteReader>& reader) {
	LazyTreeNodeList result;

	while (reader->next()) {
		const auto item = q->createTreeNode();
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

LazyTreeNodeList LazyNodesDataProviderDb::get(const QVariant& parentId) const {
	auto conn = d->databasesService->connection(d->databaseName);
	if (!conn) {
		return LazyTreeNodeList();
	}

	QString where;
	if (parentId.isNull()) where = "is null";
	else where = QString("= %1").arg(parentId.toInt());
	auto query = QString("%1\nwhere parent_id %2")
		.arg(d->sqlSelect)
		.arg(where);

	auto reader = conn->executeQuery(query);
	if (!reader) {
		qCritical() << "LazyNodesDataProviderDb::treeNodes. Query error:" << conn->lastError();
		return LazyTreeNodeList();
	}

	return d->readNodesList(reader);
}

LazyTreeNodeList LazyNodesDataProviderDb::search(const QString& text) const {
	auto conn = d->databasesService->connection(d->databaseName);
	if (!conn) {
		return LazyTreeNodeList();
	}

	auto reader = conn->executeQuery(d->sqlSearch);
	if (!reader) {
		qCritical() << "LazyNodesDataProviderDb::tsearchTeeNodes. Query error:" << conn->lastError();
		return LazyTreeNodeList();
	}

	reader->bindValue(":search_text", text);
	auto nodes = d->readNodesList(reader);
	auto result = LazyTreeBuilder::buildFromFlat(nodes);
	return result;
}

bool LazyNodesDataProviderDb::remove(const QVariant& id) {
	auto conn = d->databasesService->connection(d->databaseName);
	if (!conn) {
		return false;
	}
	auto deleteItem = conn->prepare(d->sqlDelete);
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

void LazyNodesDataProviderDb::Private::writeNodeProperties(std::unique_ptr<SQLiteReader>& reader, const QVariantMap& properties) {
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

bool LazyNodesDataProviderDb::update(const LazyTreeNodePtr& node) {
	auto conn = d->databasesService->connection(d->databaseName);
	if (!conn) {
		return false;
	}

	auto updateItem = conn->prepare(d->sqlDelete);
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

bool LazyNodesDataProviderDb::add(const LazyTreeNodePtr& node) {
	auto conn = d->databasesService->connection(d->databaseName);
	if (!conn) {
		return false;
	}
	auto insertItem = conn->prepare(d->sqlInsert);
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

	// node->setId(QVariant::fromValue((int)insertItem->lastInsert()));
	onProcessNodeAfterInsert(node, insertItem->lastInsert());
	return true;
}

