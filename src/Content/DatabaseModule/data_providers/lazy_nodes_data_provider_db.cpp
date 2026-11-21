#include "Content/DatabaseModule/data_providers/lazy_nodes_data_provider_db.h"
#include "Content/DatabaseModule/data_providers/data_blobs_provider_db.h"
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
#include <QUuid>

namespace {

/// Выборка уровня дерева: непосредственные дети запрошенного узла и, сразу,
/// вложенные узлы веток с признаком «узел всегда открыт».
///
/// Рекурсивная часть спускается от загруженного узла p к его детям c, пока у p
/// выставлено expanded: если проверять expanded у самого ребёнка (c.expanded),
/// то у раскрытого узла пропали бы нераскрытые дети, поэтому условие стоит на
/// родителе. UNION (без ALL) отсекает повторы — повреждённые данные с циклом в
/// parent_id не приводят к бесконечной рекурсии. Ветка без признака остаётся
/// ленивой: узел приходит с пустым списком детей и грузит их при раскрытии.
const QString kSqlSelectTemplate = R"(
	with recursive
	loaded as (
		select
			t.id,
			t.parent_id,
			t.name,
			t.data,
			t.expanded,
			case
				when exists (
					select 1
					from {{table}} as child
					where child.parent_id = t.id
				)
				then 1
				else 0
			end as has_children
		from {{table}} as t
		where {{scope}}
		union
		-- Вложенные узлы раскрытой ветки.
		select
			c.id,
			c.parent_id,
			c.name,
			c.data,
			c.expanded,
			case
				when exists (
					select 1
					from {{table}} as child
					where child.parent_id = c.id
				)
				then 1
				else 0
			end as has_children
		from {{table}} as c
		join loaded as p on c.parent_id = p.id and p.expanded = 1
	)
	select
		loaded.id,
		loaded.parent_id,
		loaded.name,
		loaded.data,
		loaded.has_children,
		loaded.expanded
	from loaded
	order by loaded.name collate nocase, loaded.id
)";

const QString kSqlSelectByIdTemplate = R"(
	select
		t.id,
		t.parent_id,
		t.name,
		t.data,
		t.expanded,
		case
			when exists (
				select 1
				from {{table}} as child
				where child.parent_id = t.id
			)
			then 1
			else 0
		end as has_children
	from {{table}} as t
	where t.id = :id
)";

const QString kSqlDeleteTemplate = R"(
	delete from {{table}} where id = :id
)";

/// Обновление узла. Признак expanded здесь не меняется: update() вызывается и
/// для узлов из результатов поиска, где раскрытие вычислено запросом и не
/// должно попадать в базу. Признак сохраняет kSqlUpdateExpandedTemplate.
const QString kSqlUpdateTemplate = R"(
	update {{table}} set
		parent_id = :parent_id,
		name = :name,
		data = :data
	where id = :id
)";

const QString kSqlUpdateExpandedTemplate = R"(
	update {{table}} set expanded = :expanded where id = :id
)";

const QString kSqlInsertTemplate = R"(
	insert into {{table}} (parent_id, name, data, expanded)
	values (:parent_id, :name, :data, :expanded)
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

/// Текстовое представление значения для таблицы data_blobs.
QString propertyValueToBlobText(const QVariant& value) {
	if (!value.isValid() || value.isNull()) {
		return {};
	}

	const auto type = value.metaType().id();
	if (type == QMetaType::QVariantMap || type == QMetaType::QVariantList) {
		return QString::fromUtf8(
			QJsonDocument::fromVariant(value).toJson(QJsonDocument::Compact));
	}
	if (type == QMetaType::QColor) {
		return value.value<QColor>().name(QColor::HexArgb);
	}
	if (type == QMetaType::QByteArray) {
		return QString::fromUtf8(value.toByteArray());
	}
	if (type == QMetaType::Bool) {
		return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
	}
	if (type == QMetaType::Int || type == QMetaType::LongLong
		|| type == QMetaType::UInt || type == QMetaType::ULongLong) {
		return QString::number(value.toLongLong());
	}
	if (type == QMetaType::Double || type == QMetaType::Float) {
		return QString::number(value.toDouble(), 'g', 15);
	}

	return value.toString();
}

/// Восстанавливает значение свойства из текста data_blobs по mime-типам.
QVariant propertyValueFromBlobText(const QString& text, const QStringList& mimeTypes) {
	const auto primary = PropertyMime::primaryMimeType(mimeTypes);
	const auto type = PropertyMime::editorType(primary);

	if (type == PropertyType::Boolean) {
		return text.trimmed().compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0;
	}
	if (type == PropertyType::Number) {
		bool ok = false;
		if (PropertyMime::isInteger(primary)) {
			const auto number = text.toLongLong(&ok);
			return ok ? QVariant(number) : QVariant(0);
		}

		const auto number = text.toDouble(&ok);
		return ok ? QVariant(number) : QVariant(0.0);
	}
	if (type == PropertyType::Color) {
		const QColor color(text);
		if (color.isValid()) {
			return color;
		}
	}

	return text;
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
		blobs = std::make_unique<DataBlobsProviderDb>(databasesService, databaseName);
		const QString quotedTable = quoteIdentifier(tableName);

		const auto build = [&quotedTable](const QString& sql, const QString& scope = {}) {
			QString result = sql;
			result.replace("{{table}}", quotedTable);
			result.replace("{{scope}}", scope);
			return result;
		};

		sqlSelectRoots = build(kSqlSelectTemplate, QStringLiteral("t.parent_id is null"));
		sqlSelectChildren = build(kSqlSelectTemplate,
			QStringLiteral("t.parent_id = :parent_id"));
		sqlSelectById = build(kSqlSelectByIdTemplate);
		sqlInsert = build(kSqlInsertTemplate);
		sqlDelete = build(kSqlDeleteTemplate);
		sqlUpdate = build(kSqlUpdateTemplate);
		sqlUpdateExpanded = build(kSqlUpdateExpandedTemplate);
		sqlSearch = build(kSqlSearchTemplate);
	}

	LazyNodesDataProviderDb* q;
	DatabasesService* databasesService;
	QString databaseName;
	QString tableName;
	std::unique_ptr<DataBlobsProviderDb> blobs;

	QString sqlSelectRoots;
	QString sqlSelectChildren;
	QString sqlSelectById;
	QString sqlInsert;
	QString sqlDelete;
	QString sqlUpdate;
	QString sqlUpdateExpanded;
	QString sqlSearch;

	mutable QString lastError;

	SQLiteConnection* connection() const;
	LazyTreeNodeList readNodes(const std::unique_ptr<SQLiteReader>& reader) const;
	void readProperties(const std::unique_ptr<SQLiteReader>& reader, const LazyTreeNodePtr& node) const;
	bool bindProperties(SQLiteReader& reader, const LazyTreeNodePtr& node);
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
			// Дети есть, но по умолчанию их нужно загружать отдельным
			// запросом. Узлы, пришедшие в составе раскрытой ветки, получат
			// готовый список детей при сборке дерева в get().
			node->setChildren(LazyTreeNodeList());
		}

		result.push_back(node);
	}

	return result;
}

void LazyNodesDataProviderDb::Private::readProperties(
	const std::unique_ptr<SQLiteReader>& reader, const LazyTreeNodePtr& node) const {
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
			|| entryObject.contains(QStringLiteral("value_blob"))
			|| entryObject.contains(QStringLiteral("mime"));
		if (!structured) {
			node->setProperty(name, entry.toVariant());
			continue;
		}

		const auto mimeTypes = mimeTypesFromJson(entryObject.value(QStringLiteral("mime")));
		node->setPropertyMimeTypes(name, mimeTypes);

		const auto blobId = entryObject.value(QStringLiteral("value_blob")).toString().trimmed();
		if (!blobId.isEmpty()) {
			node->setPropertyValueBlobId(name, blobId);
		}

		// Если записаны оба поля, приоритет у value.
		if (entryObject.contains(QStringLiteral("value"))) {
			node->setProperty(name,
				propertyValueFromJson(entryObject.value(QStringLiteral("value")), mimeTypes));
			continue;
		}

		if (blobId.isEmpty()) {
			node->setProperty(name, QVariant());
			continue;
		}

		const auto blob = blobs ? blobs->blob(blobId) : std::nullopt;
		if (!blob.has_value()) {
			qWarning() << "LazyNodesDataProviderDb: blob not found" << blobId;
			node->setProperty(name, QVariant());
			continue;
		}

		node->setProperty(name, propertyValueFromBlobText(blob->data, mimeTypes));
	}
}

bool LazyNodesDataProviderDb::Private::bindProperties(SQLiteReader& reader,
	const LazyTreeNodePtr& node) {
	lastError.clear();

	if (!node || node->properties().isEmpty()) {
		reader.bindValue(":data", QStringLiteral("{}"));
		return true;
	}

	QJsonObject root;
	const auto properties = node->properties();
	for (auto it = properties.constBegin(); it != properties.constEnd(); it++) {
		const auto name = it.key();
		const auto mimeTypes = node->propertyMimeTypes(name);
		QJsonObject entry;
		entry.insert(QStringLiteral("mime"), mimeTypesToJson(mimeTypes));

		if (node->propertyValueIsBlob(name)) {
			auto blobId = node->propertyValueBlobId(name).trimmed();
			if (blobId.isEmpty()) {
				blobId = QUuid::createUuid().toString(QUuid::WithoutBraces);
				node->setPropertyValueBlobId(name, blobId);
			}

			DataBlob blob;
			blob.id = blobId;
			if (blobs) {
				const auto existing = blobs->blob(blobId);
				if (existing.has_value()) {
					blob.version = existing->version;
					blob.description = existing->description;
				}
			}
			blob.data = propertyValueToBlobText(it.value());

			if (!blobs || !blobs->saveBlob(blob)) {
				lastError = blobs ? blobs->lastError()
					: QStringLiteral("Поставщик блоков данных не задан");
				qWarning() << "LazyNodesDataProviderDb: blob save failed" << blobId << lastError;
				return false;
			}

			entry.insert(QStringLiteral("value_blob"), blobId);
		}
		else {
			entry.insert(QStringLiteral("value"), propertyValueToJson(it.value()));
		}

		root.insert(name, entry);
	}

	const QJsonDocument document(root);
	const QString text = QString::fromUtf8(document.toJson(QJsonDocument::Compact));
	reader.bindValue(":data", text);
	return true;
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

	std::unique_ptr<SQLiteReader> reader;
	if (roots) {
		reader = connection->executeQuery(d->sqlSelectRoots);
	}
	else {
		reader = connection->prepare(d->sqlSelectChildren);
		if (reader) {
			reader->bindValue(":parent_id", parentId);
		}
	}

	if (!reader) {
		d->lastError = connection->lastError();
		return {};
	}

	// Запрос возвращает не только детей запрошенного узла, но и вложенные
	// узлы раскрытых веток: собираем из плоского списка дерево. Узлы, чьи
	// дети не пришли, остаются ленивыми (пустой список в readNodes).
	return LazyTreeBuilder::buildFromFlat(d->readNodes(reader));
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

	if (!d->bindProperties(*statement, node)) {
		return false;
	}
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
	statement->bindValue(":expanded", node->expanded() ? 1 : 0);
	if (!d->bindProperties(*statement, node)) {
		return false;
	}

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

bool LazyNodesDataProviderDb::setExpanded(const QVariant& id, bool expanded) {
	if (!id.isValid() || id.isNull()) {
		return false;
	}

	auto* connection = d->connection();
	if (!connection) {
		return false;
	}

	auto statement = connection->prepare(d->sqlUpdateExpanded);
	if (!statement) {
		d->lastError = connection->lastError();
		return false;
	}

	statement->bindValue(":expanded", expanded ? 1 : 0);
	statement->bindValue(":id", id);

	if (!statement->exec()) {
		d->lastError = connection->lastError();
		qWarning() << "LazyNodesDataProviderDb: expanded update failed for id" << id
			<< d->lastError;
		return false;
	}

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
