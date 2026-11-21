#include "Content/DatabaseModule/data_providers/lazy_nodes_data_provider_db.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"
#include "Libs/Resources/resources.h"

#include <QColor>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

namespace {

constexpr qint64 kMigrationVersion = 20261119180000LL;

const QString kMigrationUp = QStringLiteral(R"(
CREATE TABLE "menu_items" (
	"id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	"parent_id" INTEGER,
	"name" TEXT NOT NULL,
	"data" TEXT NOT NULL DEFAULT '{}',
	"expanded" INTEGER NOT NULL DEFAULT 0,
	CONSTRAINT "FK_menu_items_parent_id" FOREIGN KEY (parent_id) REFERENCES "menu_items"(id) ON DELETE CASCADE ON UPDATE CASCADE
);
CREATE INDEX "IX_menu_items_parent_id" ON "menu_items"("parent_id");
)");

class TestController : public CommandController {
public:
	explicit TestController(Resources* resources)
		: CommandController(resources) {
	}

	CommandProcessor* commandProcessor() const override {
		return nullptr;
	}

	ServicesRegistry* services() const override {
		return nullptr;
	}

private:
	void initContext() override {
	}
};

LazyTreeNodePtr makeNode(const QString& name, const QVariant& parentId = {}) {
	auto node = std::make_shared<LazyTreeDataNode>();
	node->setName(name);
	node->setParentId(parentId);
	return node;
}

} // namespace

class TestLazyNodesDataProviderDb : public QObject {
	Q_OBJECT
private slots:
	void init();
	void cleanup();

	void addsRootAndChild();
	void loadsExpandedBranch();
	void setExpandedPersistsFlag();
	void updateKeepsRow();
	void updateMovesNode();
	void searchBuildsHierarchy();
	void removeCascadesChildren();
	void getNodeReturnsProperties();
	void storesPropertyMimeTypes();
	void readsPropertyMimeObject();
	void emptyPropertiesRoundTrip();
	void lastErrorForMissingTable();

private:
	std::unique_ptr<QTemporaryDir> _directory;
	std::unique_ptr<Resources> _resources;
	std::unique_ptr<TestController> _controller;
	std::shared_ptr<CommandContext> _menuContext;
	std::unique_ptr<DatabasesService> _databases;
	std::unique_ptr<LazyNodesDataProviderDb> _provider;

	LazyTreeNodePtr addNode(const QString& name, const QVariant& parentId = {});
};

void TestLazyNodesDataProviderDb::init() {
	_directory = std::make_unique<QTemporaryDir>();
	QVERIFY(_directory->isValid());

	_resources = std::make_unique<Resources>();
	_resources->Directories.set("path.databases", _directory->path());

	_controller = std::make_unique<TestController>(_resources.get());
	auto store = _controller->globalContext()->createScopedContext("store");
	_menuContext = store->createScopedContext("system_menu");

	QVariantMap migration;
	migration.insert("version", QVariant::fromValue(kMigrationVersion));
	migration.insert("description", QStringLiteral("Initialize system menu"));
	migration.insert("up", kMigrationUp);
	migration.insert("down", QStringLiteral("DROP TABLE \"menu_items\";"));
	_menuContext->setVariable("migrations", QVariantList{ migration });

	_databases = std::make_unique<DatabasesService>(_resources.get(), _controller->globalContext());
	_provider = std::make_unique<LazyNodesDataProviderDb>(_databases.get(),
		QStringLiteral("system_menu"), QStringLiteral("menu_items"));
}

void TestLazyNodesDataProviderDb::cleanup() {
	_provider.reset();
	_databases.reset();
	_menuContext.reset();
	_controller.reset();
	_resources.reset();
	_directory.reset();
}

LazyTreeNodePtr TestLazyNodesDataProviderDb::addNode(const QString& name, const QVariant& parentId) {
	auto node = _provider->createTreeNode();
	node->setName(name);
	node->setParentId(parentId);
	if (!_provider->add(node)) {
		return nullptr;
	}

	return node;
}

void TestLazyNodesDataProviderDb::addsRootAndChild() {
	auto root = addNode(QStringLiteral("Корень"));
	QVERIFY(root);
	QVERIFY(root->id().isValid());

	root->setProperty(QStringLiteral("icon"), QStringLiteral("📁"));
	QVERIFY(_provider->update(root));

	// Пока детей нет, узел — лист.
	auto roots = _provider->get(QVariant());
	QCOMPARE(roots.size(), size_t(1));
	QCOMPARE(roots.front()->name(), QStringLiteral("Корень"));
	QCOMPARE(roots.front()->properties().value(QStringLiteral("icon")).toString(), QStringLiteral("📁"));
	QVERIFY(!roots.front()->children().has_value());

	auto child = addNode(QStringLiteral("Потомок"), root->id());
	QVERIFY(child);

	// Появился ребёнок: узел помечается как ленивый (пустой список детей).
	roots = _provider->get(QVariant());
	QCOMPARE(roots.size(), size_t(1));
	QVERIFY(roots.front()->children().has_value());
	QVERIFY(roots.front()->children()->empty());

	const auto children = _provider->get(root->id());
	QCOMPARE(children.size(), size_t(1));
	QCOMPARE(children.front()->name(), QStringLiteral("Потомок"));
	QCOMPARE(children.front()->parentId().toLongLong(), root->id().toLongLong());
}

void TestLazyNodesDataProviderDb::loadsExpandedBranch() {
	auto root = addNode(QStringLiteral("Ветка"));
	QVERIFY(root);
	auto child = addNode(QStringLiteral("Ребёнок"), root->id());
	QVERIFY(child);
	auto grandchild = addNode(QStringLiteral("Внук"), child->id());
	QVERIFY(grandchild);
	QVERIFY(addNode(QStringLiteral("Сосед"), root->id()));

	// Пока признак не задан, дерево остаётся ленивым.
	auto roots = _provider->get(QVariant());
	QCOMPARE(roots.size(), size_t(1));
	QVERIFY(roots.front()->children().has_value());
	QVERIFY(roots.front()->children()->empty());
	QVERIFY(!roots.front()->expanded());

	// «Узел всегда открыт» у корня: сразу приходят все его дети.
	QVERIFY(_provider->setExpanded(root->id(), true));
	roots = _provider->get(QVariant());
	QCOMPARE(roots.size(), size_t(1));
	QVERIFY(roots.front()->expanded());
	QVERIFY(roots.front()->children().has_value());
	QCOMPARE(roots.front()->children()->size(), size_t(2));

	// Вложенный узел не раскрыт: его дети грузятся при раскрытии.
	const auto nested = roots.front()->children()->front();
	QCOMPARE(nested->name(), QStringLiteral("Ребёнок"));
	QVERIFY(nested->children().has_value());
	QVERIFY(nested->children()->empty());
	QVERIFY(!nested->expanded());

	// Раскрытая вложенная ветка приходит целиком, до листа.
	QVERIFY(_provider->setExpanded(child->id(), true));
	roots = _provider->get(QVariant());
	QVERIFY(roots.front()->children().has_value());
	QCOMPARE(roots.front()->children()->size(), size_t(2));
	const auto branch = roots.front()->children()->front();
	QVERIFY(branch->expanded());
	QVERIFY(branch->children().has_value());
	QCOMPARE(branch->children()->size(), size_t(1));
	QCOMPARE(branch->children()->front()->name(), QStringLiteral("Внук"));
	QVERIFY(!branch->children()->front()->children().has_value());
	QVERIFY(!branch->children()->front()->expanded());

	// Признак читается и при точечной загрузке узла.
	QVERIFY(_provider->getNode(root->id())->expanded());
	QVERIFY(_provider->getNode(child->id())->expanded());
	QVERIFY(!_provider->getNode(grandchild->id())->expanded());
}

void TestLazyNodesDataProviderDb::setExpandedPersistsFlag() {
	auto node = addNode(QStringLiteral("Узел"));
	QVERIFY(node);
	QVERIFY(!_provider->getNode(node->id())->expanded());

	QVERIFY(_provider->setExpanded(node->id(), true));
	QVERIFY(_provider->getNode(node->id())->expanded());

	// update() не трогает признак: служебное раскрытие из результатов поиска
	// не должно попадать в базу.
	auto loaded = _provider->getNode(node->id());
	QVERIFY(loaded);
	loaded->setExpanded(false);
	loaded->setName(QStringLiteral("Переименованный"));
	QVERIFY(_provider->update(loaded));
	loaded = _provider->getNode(node->id());
	QVERIFY(loaded);
	QCOMPARE(loaded->name(), QStringLiteral("Переименованный"));
	QVERIFY(loaded->expanded());

	QVERIFY(_provider->setExpanded(node->id(), false));
	QVERIFY(!_provider->getNode(node->id())->expanded());

	// Новый узел сохраняется вместе с признаком.
	auto expanded = _provider->createTreeNode();
	expanded->setName(QStringLiteral("Раскрытая"));
	expanded->setExpanded(true);
	QVERIFY(_provider->add(expanded));
	QVERIFY(_provider->getNode(expanded->id())->expanded());
}

void TestLazyNodesDataProviderDb::updateKeepsRow() {
	auto root = addNode(QStringLiteral("Корень"));
	QVERIFY(root);

	root->setName(QStringLiteral("Обновлённый"));
	root->setProperty(QStringLiteral("color"), QStringLiteral("red"));
	QVERIFY(_provider->update(root));

	const auto roots = _provider->get(QVariant());
	QCOMPARE(roots.size(), size_t(1));
	QCOMPARE(roots.front()->name(), QStringLiteral("Обновлённый"));
	QCOMPARE(roots.front()->properties().value(QStringLiteral("color")).toString(), QStringLiteral("red"));
}

void TestLazyNodesDataProviderDb::updateMovesNode() {
	auto first = addNode(QStringLiteral("Первый"));
	auto second = addNode(QStringLiteral("Второй"));
	QVERIFY(first);
	QVERIFY(second);

	auto child = addNode(QStringLiteral("Потомок"), first->id());
	QVERIFY(child);

	child->setParentId(second->id());
	QVERIFY(_provider->update(child));

	QCOMPARE(_provider->get(first->id()).size(), size_t(0));
	const auto moved = _provider->get(second->id());
	QCOMPARE(moved.size(), size_t(1));
	QCOMPARE(moved.front()->name(), QStringLiteral("Потомок"));
}

void TestLazyNodesDataProviderDb::searchBuildsHierarchy() {
	auto root = addNode(QStringLiteral("Корень"));
	QVERIFY(root);
	auto child = addNode(QStringLiteral("Найденный потомок"), root->id());
	QVERIFY(child);
	QVERIFY(addNode(QStringLiteral("Другой"), root->id()));

	const auto results = _provider->search(QStringLiteral("Найденный"));
	QCOMPARE(results.size(), size_t(1));
	QCOMPARE(results.front()->name(), QStringLiteral("Корень"));
	QVERIFY(results.front()->expanded());
	QVERIFY(results.front()->children().has_value());
	QCOMPARE(results.front()->children()->size(), size_t(1));
	QCOMPARE(results.front()->children()->front()->name(), QStringLiteral("Найденный потомок"));
}

void TestLazyNodesDataProviderDb::removeCascadesChildren() {
	auto root = addNode(QStringLiteral("Корень"));
	QVERIFY(root);
	auto child = addNode(QStringLiteral("Потомок"), root->id());
	QVERIFY(child);
	const auto childId = child->id();

	QVERIFY(_provider->remove(root->id()));

	QCOMPARE(_provider->get(QVariant()).size(), size_t(0));
	QVERIFY(!_provider->getNode(childId));
}

void TestLazyNodesDataProviderDb::getNodeReturnsProperties() {
	auto node = addNode(QStringLiteral("Узел"));
	QVERIFY(node);
	node->setProperty(QStringLiteral("key"), QStringLiteral("value"));
	node->setProperty(QStringLiteral("number"), 42);
	QVERIFY(_provider->update(node));

	const auto loaded = _provider->getNode(node->id());
	QVERIFY(loaded);
	QCOMPARE(loaded->name(), QStringLiteral("Узел"));
	QCOMPARE(loaded->properties().value(QStringLiteral("key")).toString(), QStringLiteral("value"));
	QCOMPARE(loaded->properties().value(QStringLiteral("number")).toInt(), 42);
	QVERIFY(!_provider->getNode(999999));
}

void TestLazyNodesDataProviderDb::storesPropertyMimeTypes() {
	auto node = addNode(QStringLiteral("Узел"));
	QVERIFY(node);

	node->setProperty(QStringLiteral("color"), QColor(Qt::red));
	node->setPropertyMimeTypes(QStringLiteral("color"),
		{ QStringLiteral("application/x-color"), QStringLiteral("text/plain") });
	node->setProperty(QStringLiteral("title"), QStringLiteral("Заголовок"));
	node->setPropertyMimeTypes(QStringLiteral("title"),
		{ QStringLiteral("text/plain"), QStringLiteral("text/x-multiline") });
	QVERIFY(_provider->update(node));

	const auto loaded = _provider->getNode(node->id());
	QVERIFY(loaded);
	QCOMPARE(loaded->propertyMimeTypes(QStringLiteral("color")),
		QStringList({ QStringLiteral("application/x-color"), QStringLiteral("text/plain") }));
	QCOMPARE(loaded->propertyMimeTypes(QStringLiteral("title")),
		QStringList({ QStringLiteral("text/plain"), QStringLiteral("text/x-multiline") }));
	QVERIFY(loaded->properties().value(QStringLiteral("color")).value<QColor>().isValid());

	// Формат колонки data: { "property": { "mime": [...], "value": ... } }.
	auto* connection = _databases->connection(QStringLiteral("system_menu"));
	QVERIFY(connection);
	auto reader = connection->executeQuery(
		QStringLiteral("select data from menu_items where id = %1").arg(node->id().toLongLong()));
	QVERIFY(reader);
	QVERIFY(reader->next());
	const auto object = QJsonDocument::fromJson(reader->value(QStringLiteral("data")).toString().toUtf8()).object();
	const auto colorEntry = object.value(QStringLiteral("color")).toObject();
	QVERIFY(colorEntry.value(QStringLiteral("mime")).isArray());
	QCOMPARE(colorEntry.value(QStringLiteral("value")).toString(),
		QColor(Qt::red).name(QColor::HexArgb));

	QVERIFY(loaded->removeProperty(QStringLiteral("title")));
	QVERIFY(_provider->update(loaded));
	const auto afterRemove = _provider->getNode(node->id());
	QVERIFY(afterRemove);
	QVERIFY(!afterRemove->properties().contains(QStringLiteral("title")));
	QVERIFY(afterRemove->propertyMimeTypes(QStringLiteral("title")).isEmpty());
}

void TestLazyNodesDataProviderDb::readsPropertyMimeObject() {
	auto node = addNode(QStringLiteral("Узел"));
	QVERIFY(node);

	// Требование допускает mime как объект, а не только массив.
	QJsonObject mimeObject;
	mimeObject.insert(QStringLiteral("image/png"), true);
	QJsonObject entry;
	entry.insert(QStringLiteral("value"), QStringLiteral("image.png"));
	entry.insert(QStringLiteral("mime"), mimeObject);
	QJsonObject data;
	data.insert(QStringLiteral("image"), entry);

	auto* connection = _databases->connection(QStringLiteral("system_menu"));
	QVERIFY(connection);
	auto update = connection->prepare(
		QStringLiteral("update menu_items set data = :data where id = :id"));
	QVERIFY(update);
	update->bindValue(QStringLiteral(":data"),
		QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
	update->bindValue(QStringLiteral(":id"), node->id());
	QVERIFY(update->exec());

	const auto loaded = _provider->getNode(node->id());
	QVERIFY(loaded);
	QCOMPARE(loaded->propertyMimeTypes(QStringLiteral("image")),
		QStringList({ QStringLiteral("image/png") }));
	QCOMPARE(loaded->properties().value(QStringLiteral("image")).toString(),
		QStringLiteral("image.png"));
}


void TestLazyNodesDataProviderDb::emptyPropertiesRoundTrip() {
	auto node = addNode(QStringLiteral("Узел"));
	QVERIFY(node);

	const auto loaded = _provider->getNode(node->id());
	QVERIFY(loaded);
	QVERIFY(loaded->properties().isEmpty());
}

void TestLazyNodesDataProviderDb::lastErrorForMissingTable() {
	LazyNodesDataProviderDb broken(_databases.get(), QStringLiteral("system_menu"),
		QStringLiteral("missing_table"));

	const auto nodes = broken.get(QVariant());
	QVERIFY(nodes.empty());
	QVERIFY(!broken.lastError().isEmpty());
}

QTEST_MAIN(TestLazyNodesDataProviderDb)
#include "test_lazy_nodes_data_provider_db.moc"
