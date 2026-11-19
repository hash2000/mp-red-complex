#include "Content/DatabaseModule/data_providers/lazy_nodes_data_provider_db.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/i_lazy_tree_node.h"
#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_data_node.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Resources/resources.h"

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
	void updateKeepsRow();
	void updateMovesNode();
	void searchBuildsHierarchy();
	void removeCascadesChildren();
	void getNodeReturnsProperties();
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
