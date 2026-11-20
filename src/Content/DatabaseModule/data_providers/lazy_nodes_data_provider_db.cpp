#include "Content/DatabaseModule/data_providers/lazy_nodes_data_provider_db.h"
#include "Libs/BaseWidgets/properties/property_mime.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_builder.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"

#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"

#include <QColor>
#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QSet>

namespace {

const QString kSqlSelectTemplate = R"(
	select
		t.id,
		t.parent_id,
		t.name,
		t.data,
		case
			when exists (
				select 1
				from {{table}} as child
				where child.parent_id = t.id
			)
			then 1
			else 0
		end as has_children,
		0 as expanded
	from {{table}} as t
)";

const QString kSqlSelectByIdTemplate = R"(
	select
		t.id,
		t.parent_id,
		t.name,
		t.data,
		case
			when exists (
				select 1
				from {{table}} as child
				where child.parent_id = t.id
			)
			then 1
			else 0
		end as has_children,
		0 as expanded
	from {{table}} as t
	where t.id = :id
)";

const QString kSqlDeleteTemplate = R"(
	delete from {{table}} where id = :id
)";

const QString kSqlUpdateTemplate = R"(
	update {{table}} set
		parent_id = :parent_id,
		name = :name,
		data = :data
	where id = :id
)";

const QString kSqlInsertTemplate = R"(
	insert into {{table}} (parent_id, name, data) values (:parent_id, :name, :data)
)";

const QString kSqlSearchTemplate = R"(
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
		from {{table}} as qt
		where qt.name like '%' || :search_text || '%'
	),
	raw as (
		-- Найденные узлы изначально считаются закрытыми.
		select
			id,
			parent_id,
			name,
			has_children,
			0 as expanded
		from found
		union all
		-- Предки найденных узлов должны быть раскрыты.
		select
			p.id,
			p.parent_id,
			p.name,
			1 as has_children,
			1 as expanded
		from {{table}} as p
		join raw as c on c.parent_id = p.id
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
	join {{table}} as q on q.id = cte.id
	order by q.name collate nocase, q.id
)";

QString quoteIdentifier(const QString& identifier) {
	QString quoted = identifier;
	quoted.replace('"', "\"\"");
	return QStringLiteral("\"%1\"").arg(quoted);
}

/// Mime-типы свойства в JSON. Каноническая запись — массив строк.
QJsonArray mimeTypesToJson(const QStringList& mimeTypes) {
	QJsonArray result;
	QSet<QString> seen;

	for (const auto& mimeType : mimeTypes) {
		const auto trimmed = mimeType.trimmed();
		if (trimmed.isEmpty() || seen.contains(trimmed)) {
			continue;
		}

		seen.insert(trimmed);
		result.append(trimmed);
	}

	return result;
}

/// Чтение mime-типов свойства. Поддерживаются оба варианта из требований:
/// массив строк и объект, где mime-типы — ключи (или значения при числовых
/// ключах, например после преобразования списка в объект).
QStringList mimeTypesFromJson(const QJsonValue& value) {
	QStringList result;
	QSet<QString> seen;

	const auto add = [&result, &seen](const QString& raw) {
		const auto trimmed = raw.trimmed();
		if (trimmed.isEmpty() || seen.contains(trimmed)) {
			return;
		}

		seen.insert(trimmed);
		result.append(trimmed);
	};

	if (value.isArray()) {
		for (const auto& item : value.toArray()) {
			if (item.isString()) {
				add(item.toString());
			}
		}
	}
	else if (value.isObject()) {
		const auto object = value.toObject();
		bool numericKeys = !object.isEmpty();
		for (auto it = object.constBegin(); it != object.constEnd(); it++) {
			bool ok = false;
			it.key().toInt(&ok);
			if (!ok) {
				numericKeys = false;
				break;
			}
		}

		for (auto it = object.constBegin(); it != object.constEnd(); it++) {
			if (numericKeys) {
				if (it.value().isString()) {
					add(it.value().toString());
				}
			}
			else {
				add(it.key());
			}
		}
	}
	else if (value.isString()) {
		add(value.toString());
	}

	return result;
}

QJsonValue propertyValueToJson(const QVariant& value) {
	if (!value.isValid() || value.isNull()) {
		return QJsonValue::Null;
	}

	// QColor не сериализуется QJsonValue напрямую: храним цвет строкой.
	if (value.metaType().id() == QMetaType::QColor) {
		return QJsonValue(value.value<QColor>().name(QColor::HexArgb));
	}

	if (value.metaType().id() == QMetaType::QByteArray) {
		return QJsonValue(QString::fromLatin1(value.toByteArray().toBase64()));
	}

	const auto json = QJsonValue::fromVariant(value);
	return json.isUndefined() ? QJsonValue(value.toString()) : json;
}

QVariant propertyValueFromJson(const QJsonValue& value, const QStringList& mimeTypes) {
	if (value.isNull() || value.isUndefined()) {
		return {};
	}

	const auto primary = PropertyMime::primaryMimeType(mimeTypes);
	if (PropertyMime::editorType(primary) == PropertyType::Color && value.isString()) {
		const QColor color(value.toString());
		if (color.isValid()) {
			return color;
		}
	}

	return value.toVariant();
}

} // namespace

class LazyNodesDataProviderDb::Private {
public:
	Private(LazyNodesDataProviderDb* parent, DatabasesService* databasesService,
		const QString& databaseName, const QString& tableName)
		: q(parent)
		, databasesService(databasesService)
		, databaseName(databaseName)
		, tableName(tableName) {
		const QString quotedTable = quoteIdentifier(tableName);

		const auto build = [&quotedTable](const QString& sql) {
			QString result = sql;
			return result.replace("{{table}}", quotedTable);
		};

		sqlSelect = build(kSqlSelectTemplate);
		sqlSelectById = build(kSqlSelectByIdTemplate);
		sqlInsert = build(kSqlInsertTemplate);
		sqlDelete = build(kSqlDeleteTemplate);
		sqlUpdate = build(kSqlUpdateTemplate);
		sqlSearch = build(kSqlSearchTemplate);
	}

	LazyNodesDataProviderDb* q;
	DatabasesService* databasesService;
	QString databaseName;
	QString tableName;

	QString sqlSelect;
	QString sqlSelectById;
	QString sqlInsert;
	QString sqlDelete;
	QString sqlUpdate;
	QString sqlSearch;

	mutable QString lastError;

	SQLiteConnection* connection() const;
	LazyTreeNodeList readNodes(const std::unique_ptr<SQLiteReader>& reader) const;
	static void readProperties(const std::unique_ptr<SQLiteReader>& reader, const LazyTreeNodePtr& node);
	static void bindProperties(SQLiteReader& reader, const LazyTreeNodePtr& node);
};

SQLiteConnection* LazyNodesDataProviderDb::Private::connection() const {
	lastError.clear();

	if (!databasesService) {
		lastError = QStringLiteral("Сервис баз данных не задан");
		return nullptr;
	}

	auto* result = databasesService->connection(databaseName);
	if (!result) {
		lastError = QStringLiteral("Не удалось открыть хранилище «%1»").arg(databaseName);
		qWarning() << "LazyNodesDataProviderDb:" << lastError;
	}

	return result;
}

LazyTreeNodeList LazyNodesDataProviderDb::Private::readNodes(
	const std::unique_ptr<SQLiteReader>& reader) const {
	LazyTreeNodeList result;

	if (!reader) {
		return result;
	}

	while (reader->next()) {
		const auto node = q->createTreeNode();
		if (!node) {
			continue;
		}

		node->setId(reader->value("id"));
		node->setName(reader->value("name").toString());
		node->setExpanded(reader->value("expanded").toInt() != 0);

		const auto parentId = reader->value("parent_id");
		if (!parentId.isNull()) {
			node->setParentId(parentId);
		}

		readProperties(reader, node);

		if (reader->value("has_children").toLongLong() > 0) {
			// Дети есть, но их нужно загрузить отдельным запросом.
			node->setChildren(LazyTreeNodeList());
		}

		result.push_back(node);
	}

	return result;
}

void LazyNodesDataProviderDb::Private::readProperties(
	const std::unique_ptr<SQLiteReader>& reader, const LazyTreeNodePtr& node) {
	const QString raw = reader->value("data").toString();
	if (raw.isEmpty()) {
		return;
	}

	QJsonParseError error;
	const QJsonDocument document = QJsonDocument::fromJson(raw.toUtf8(), &error);
	if (error.error != QJsonParseError::NoError) {
		qWarning() << "LazyNodesDataProviderDb: JSON parse error:" << error.errorString();
		return;
	}

	if (!document.isObject()) {
		qWarning() << "LazyNodesDataProviderDb: expected JSON object";
		return;
	}

	const auto object = document.object();
	for (auto it = object.constBegin(); it != object.constEnd(); it++) {
		const auto name = it.key();
		const auto entry = it.value();

		// Старый формат: значение свойства лежало прямо в JSON.
		if (!entry.isObject()) {
			node->setProperty(name, entry.toVariant());
			continue;
		}

		const auto entryObject = entry.toObject();
		const bool structured = entryObject.contains(QStringLiteral("value"))
			|| entryObject.contains(QStringLiteral("mime"));
		if (!structured) {
			node->setProperty(name, entry.toVariant());
			continue;
		}

		const auto mimeTypes = mimeTypesFromJson(entryObject.value(QStringLiteral("mime")));
		node->setPropertyMimeTypes(name, mimeTypes);
		node->setProperty(name,
			propertyValueFromJson(entryObject.value(QStringLiteral("value")), mimeTypes));
	}
}

void LazyNodesDataProviderDb::Private::bindProperties(SQLiteReader& reader,
	const LazyTreeNodePtr& node) {
	if (!node || node->properties().isEmpty()) {
		reader.bindValue(":data", QStringLiteral("{}"));
		return;
	}

	QJsonObject root;
	const auto properties = node->properties();
	for (auto it = properties.constBegin(); it != properties.constEnd(); it++) {
		QJsonObject entry;
		entry.insert(QStringLiteral("value"), propertyValueToJson(it.value()));
		entry.insert(QStringLiteral("mime"),
			mimeTypesToJson(node->propertyMimeTypes(it.key())));
		root.insert(it.key(), entry);
	}

	const QJsonDocument document(root);
	const QString text = QString::fromUtf8(document.toJson(QJsonDocument::Compact));
	reader.bindValue(":data", text);
}

LazyNodesDataProviderDb::LazyNodesDataProviderDb(DatabasesService* databasesService,
	const QString& databaseName, const QString& tableName)
	: d(std::make_unique<Private>(this, databasesService, databaseName, tableName)) {
	static const QRegularExpression validTableName(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
	if (!validTableName.match(tableName).hasMatch()) {
		qWarning() << "LazyNodesDataProviderDb: suspicious table name" << tableName;
	}

	if (tableName.isEmpty()) {
		qWarning() << "LazyNodesDataProviderDb: table name is empty";
	}
}

LazyNodesDataProviderDb::~LazyNodesDataProviderDb() = default;

LazyTreeNodePtr LazyNodesDataProviderDb::createTreeNode() {
	return std::make_shared<LazyTreeDataNode>();
}

LazyTreeNodePtr LazyNodesDataProviderDb::createRootNode() {
	return std::make_shared<LazyTreeDataNode>();
}

LazyTreeNodeList LazyNodesDataProviderDb::get(const QVariant& parentId) const {
	auto* connection = d->connection();
	if (!connection) {
		return {};
	}

	const bool roots = !parentId.isValid() || parentId.isNull();

	QString query = d->sqlSelect;
	query += roots
		? QStringLiteral("\nwhere t.parent_id is null")
		: QStringLiteral("\nwhere t.parent_id = :parent_id");
	query += QStringLiteral("\norder by t.name collate nocase, t.id");

	std::unique_ptr<SQLiteReader> reader;
	if (roots) {
		reader = connection->executeQuery(query);
	}
	else {
		reader = connection->prepare(query);
		if (reader) {
			reader->bindValue(":parent_id", parentId);
		}
	}

	if (!reader) {
		d->lastError = connection->lastError();
		return {};
	}

	return d->readNodes(reader);
}

LazyTreeNodePtr LazyNodesDataProviderDb::getNode(const QVariant& id) const {
	if (!id.isValid() || id.isNull()) {
		return nullptr;
	}

	auto* connection = d->connection();
	if (!connection) {
		return nullptr;
	}

	auto reader = connection->prepare(d->sqlSelectById);
	if (!reader) {
		d->lastError = connection->lastError();
		return nullptr;
	}

	reader->bindValue(":id", id);
	const auto nodes = d->readNodes(reader);
	return nodes.empty() ? nullptr : nodes.front();
}

LazyTreeNodeList LazyNodesDataProviderDb::search(const QString& text) const {
	const auto connection = d->connection();
	if (!connection) {
		return {};
	}

	auto reader = connection->prepare(d->sqlSearch);
	if (!reader) {
		d->lastError = connection->lastError();
		return {};
	}

	reader->bindValue(":search_text", text);

	const auto nodes = d->readNodes(reader);
	return LazyTreeBuilder::buildFromFlat(nodes);
}

bool LazyNodesDataProviderDb::remove(const QVariant& id) {
	auto* connection = d->connection();
	if (!connection) {
		return false;
	}

	auto statement = connection->prepare(d->sqlDelete);
	if (!statement) {
		d->lastError = connection->lastError();
		return false;
	}

	statement->bindValue(":id", id);

	if (!statement->exec()) {
		d->lastError = connection->lastError();
		qWarning() << "LazyNodesDataProviderDb: delete failed for id" << id << d->lastError;
		return false;
	}

	return true;
}

bool LazyNodesDataProviderDb::update(const LazyTreeNodePtr& node) {
	if (!node) {
		return false;
	}

	auto* connection = d->connection();
	if (!connection) {
		return false;
	}

	auto statement = connection->prepare(d->sqlUpdate);
	if (!statement) {
		d->lastError = connection->lastError();
		return false;
	}

	Private::bindProperties(*statement, node);
	statement->bindValue(":parent_id", node->parentId());
	statement->bindValue(":name", node->name());
	statement->bindValue(":id", node->id());

	if (!statement->exec()) {
		d->lastError = connection->lastError();
		qWarning() << "LazyNodesDataProviderDb: update failed for id" << node->id() << d->lastError;
		return false;
	}

	return true;
}

bool LazyNodesDataProviderDb::add(const LazyTreeNodePtr& node) {
	if (!node) {
		return false;
	}

	auto* connection = d->connection();
	if (!connection) {
		return false;
	}

	auto statement = connection->prepare(d->sqlInsert);
	if (!statement) {
		d->lastError = connection->lastError();
		return false;
	}

	statement->bindValue(":parent_id", node->parentId());
	statement->bindValue(":name", node->name());
	Private::bindProperties(*statement, node);

	if (!statement->exec()) {
		d->lastError = connection->lastError();
		qWarning() << "LazyNodesDataProviderDb: insert failed for name" << node->name() << d->lastError;
		return false;
	}

	const qint64 lastInsertId = statement->lastInsert();
	node->setId(QVariant::fromValue(lastInsertId));
	onNodeInserted(node, lastInsertId);

	return true;
}

QString LazyNodesDataProviderDb::lastError() const {
	return d->lastError;
}

void LazyNodesDataProviderDb::onNodeInserted(const LazyTreeNodePtr& node, qint64 lastInsertId) {
	Q_UNUSED(node);
	Q_UNUSED(lastInsertId);
}

void LazyNodesDataProviderDb::setLastError(const QString& error) const {
	d->lastError = error;
}
