#include "Content/DatabaseModule/data_providers/data_blobs_provider_db.h"
#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/SystemRouter/data_providers/system_router_commands_provider_db.h"
#include "Content/SystemRouter/data_providers/system_router_data_provider_db.h"
#include "Content/SystemRouter/models/system_router_mime.h"
#include "Content/SystemRouter/commands/system_router_blobs_command.h"
#include "Content/SystemRouter/commands/system_router_routes_command.h"
#include "Content/SystemRouter/commands/system_router_save_blob_command.h"
#include "Content/SystemRouter/services/system_router_service.h"
#include "Content/SystemRouter/services/system_router_blob_service.h"
#include "Libs/Engine/services/services_registry.h"
#include "Libs/Resources/db/sqlite/sqlite_connection.h"
#include "Libs/Resources/db/sqlite/sqlite_reader.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Resources/resources.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

namespace {

constexpr qint64 kMigrationVersion = 20261120090000LL;
constexpr qint64 kRenameVersion = 20261120130000LL;
const QString kCommandId = QStringLiteral("33333333-3333-3333-3333-333333333333");

const QString kMigrationUp = QStringLiteral(R"(
CREATE TABLE "nodes" (
	"id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	"parent_id" INTEGER,
	"name" TEXT NOT NULL,
	"data" TEXT NOT NULL DEFAULT '{}',
	"expanded" INTEGER NOT NULL DEFAULT 0
);
CREATE TABLE "commands" (
	"id" TEXT NOT NULL PRIMARY KEY,
	"version" INTEGER NOT NULL DEFAULT 1,
	"description" TEXT NOT NULL DEFAULT '',
	"script" TEXT NOT NULL DEFAULT ''
);
)");

const QString kRenameUp = QStringLiteral(R"(
ALTER TABLE "commands" RENAME TO "data_blobs";
ALTER TABLE "data_blobs" RENAME COLUMN "script" TO "data";
)");

class TestServices : public ServicesRegistry {
public:
	template<typename T>
	void add(std::unique_ptr<T> service) {
		registerService<T>(std::move(service));
	}
};

class TestController : public CommandController {
public:
	explicit TestController(Resources* resources)
		: CommandController(resources) {
	}

	CommandProcessor* commandProcessor() const override {
		return nullptr;
	}

	ServicesRegistry* services() const override {
		return const_cast<TestServices*>(&_services);
	}

	TestServices& testServices() {
		return _services;
	}

private:
	void initContext() override {
	}

	mutable TestServices _services;
};

} // namespace

class TestSystemRouterCommandsDb : public QObject {
	Q_OBJECT
private slots:
	void init();
	void cleanup();

	void savesReadsAndUpdatesCommand();
	void removesCommand();
	void valueBlobRoundTrip();
	void valueWinsOverBlob();
	void blobServiceRoundTrip();
	void blobsCommand();
	void routesCommand();
	void saveBlobCommand();

private:
	std::unique_ptr<QTemporaryDir> _directory;
	std::unique_ptr<Resources> _resources;
	std::unique_ptr<TestController> _controller;
	std::shared_ptr<CommandContext> _storeContext;
	std::unique_ptr<DatabasesService> _databases;
	std::unique_ptr<SystemRouterCommandsProviderDb> _provider;
};

void TestSystemRouterCommandsDb::init() {
	_directory = std::make_unique<QTemporaryDir>();
	QVERIFY(_directory->isValid());

	_resources = std::make_unique<Resources>();
	_resources->Directories.set(QStringLiteral("path.databases"), _directory->path());

	_controller = std::make_unique<TestController>(_resources.get());
	auto store = _controller->globalContext()->createScopedContext(QStringLiteral("store"));
	_storeContext = store->createScopedContext(QStringLiteral("system_router"));

	QVariantMap migration;
	migration.insert(QStringLiteral("version"), QVariant::fromValue(kMigrationVersion));
	migration.insert(QStringLiteral("description"), QStringLiteral("Initialize system router"));
	migration.insert(QStringLiteral("up"), kMigrationUp);
	migration.insert(QStringLiteral("down"), QStringLiteral("DROP TABLE \"commands\"; DROP TABLE \"nodes\";"));

	QVariantMap rename;
	rename.insert(QStringLiteral("version"), QVariant::fromValue(kRenameVersion));
	rename.insert(QStringLiteral("description"), QStringLiteral("Rename commands to data_blobs"));
	rename.insert(QStringLiteral("up"), kRenameUp);
	rename.insert(QStringLiteral("down"), QStringLiteral(
		"ALTER TABLE \"data_blobs\" RENAME COLUMN \"data\" TO \"script\"; "
		"ALTER TABLE \"data_blobs\" RENAME TO \"commands\";"));
	_storeContext->setVariable(QStringLiteral("migrations"), QVariantList{ migration, rename });

	_databases = std::make_unique<DatabasesService>(_resources.get(),
		_controller->globalContext());
	_provider = std::make_unique<SystemRouterCommandsProviderDb>(_databases.get());
}

void TestSystemRouterCommandsDb::cleanup() {
	_provider.reset();
	_databases.reset();
	_storeContext.reset();
	_controller.reset();
	_resources.reset();
	_directory.reset();
}

void TestSystemRouterCommandsDb::savesReadsAndUpdatesCommand() {
	SystemRouterCommand command;
	command.id = QStringLiteral("{%1}").arg(kCommandId);
	command.version = 2;
	command.description = QStringLiteral("Авторизация");
	command.script = QStringLiteral("authorize url={{first-url}}");
	QVERIFY(_provider->saveCommand(command));

	const auto commands = _provider->commands();
	QCOMPARE(commands.size(), 1);
	QCOMPARE(commands.first().id, kCommandId);
	QCOMPARE(commands.first().version, 2);
	QCOMPARE(commands.first().description, QStringLiteral("Авторизация"));
	QCOMPARE(commands.first().script, QStringLiteral("authorize url={{first-url}}"));

	const auto found = _provider->command(QStringLiteral("{%1}").arg(kCommandId));
	QVERIFY(found.has_value());
	QCOMPARE(found->id, kCommandId);

	command.version = 1;
	command.script = QStringLiteral("authorize url={{first-url}}, token={{token}}");
	QVERIFY(_provider->saveCommand(command));
	QCOMPARE(_provider->commands().size(), 1);
	QCOMPARE(_provider->command(kCommandId)->script,
		QStringLiteral("authorize url={{first-url}}, token={{token}}"));
}

void TestSystemRouterCommandsDb::removesCommand() {
	SystemRouterCommand command;
	command.id = kCommandId;
	command.description = QStringLiteral("Удаляемая");
	QVERIFY(_provider->saveCommand(command));
	QVERIFY(_provider->removeCommand(kCommandId));
	QVERIFY(_provider->commands().isEmpty());
	QVERIFY(!_provider->command(kCommandId).has_value());
}

void TestSystemRouterCommandsDb::valueBlobRoundTrip() {
	SystemRouterDataProviderDb nodes(_databases.get());
	auto node = nodes.createTreeNode();
	QVERIFY(node);

	const QString script = QStringLiteral("line1\nline2 \"quoted\"");
	node->setName(QStringLiteral("Узел"));
	node->setProperty(QStringLiteral("script"), script);
	node->setPropertyMimeTypes(QStringLiteral("script"),
		{ QString::fromLatin1(SystemRouterMime::kCommandParameter),
			QStringLiteral("text/x-multiline") });
	node->setProperty(QStringLiteral("ref"), kCommandId);
	node->setPropertyMimeTypes(QStringLiteral("ref"),
		{ QString::fromLatin1(SystemRouterMime::kCommand) });

	QVERIFY(nodes.add(node));
	const auto loaded = nodes.getNode(node->id());
	QVERIFY(loaded);
	QCOMPARE(loaded->properties().value(QStringLiteral("script")).toString(), script);
	QVERIFY(!loaded->propertyValueBlobId(QStringLiteral("script")).isEmpty());
	QVERIFY(loaded->propertyValueBlobId(QStringLiteral("ref")).isEmpty());
	QCOMPARE(loaded->properties().value(QStringLiteral("ref")).toString(), kCommandId);

	DataBlobsProviderDb blobs(_databases.get(), QStringLiteral("system_router"));
	const auto blob = blobs.blob(loaded->propertyValueBlobId(QStringLiteral("script")));
	QVERIFY(blob.has_value());
	QCOMPARE(blob->data, script);
}

void TestSystemRouterCommandsDb::valueWinsOverBlob() {
	DataBlobsProviderDb blobs(_databases.get(), QStringLiteral("system_router"));
	DataBlob blob;
	blob.id = QStringLiteral("44444444-4444-4444-4444-444444444444");
	blob.data = QStringLiteral("from blob");
	QVERIFY(blobs.saveBlob(blob));

	auto* connection = _databases->connection(QStringLiteral("system_router"));
	QVERIFY(connection);
	auto statement = connection->prepare(QStringLiteral(
		"insert into nodes (name, data) values (:name, :data)"));
	QVERIFY(statement);
	statement->bindValue(QStringLiteral(":name"), QStringLiteral("node"));
	statement->bindValue(QStringLiteral(":data"), QStringLiteral(
		R"({"p":{"mime":["text/plain"],"value":"from value","value_blob":"44444444-4444-4444-4444-444444444444"}})"));
	QVERIFY(statement->exec());

	SystemRouterDataProviderDb nodes(_databases.get());
	const auto nodeId = statement->lastInsert();
	const auto loaded = nodes.getNode(QVariant::fromValue<qint64>(nodeId));
	QVERIFY(loaded);
	QCOMPARE(loaded->properties().value(QStringLiteral("p")).toString(),
		QStringLiteral("from value"));
	QCOMPARE(loaded->propertyValueBlobId(QStringLiteral("p")),
		QStringLiteral("44444444-4444-4444-4444-444444444444"));
}

void TestSystemRouterCommandsDb::blobServiceRoundTrip() {
	DataBlobsProviderDb provider(_databases.get(), QStringLiteral("system_router"));
	SystemRouterBlobService service(&provider);

	const auto id = service.createData(QStringLiteral("first"));
	QVERIFY(!id.isEmpty());
	QCOMPARE(service.data(id), QStringLiteral("first"));

	QVERIFY(service.saveData(id, QStringLiteral("second")));
	QCOMPARE(service.data(id), QStringLiteral("second"));

	QVERIFY(service.removeBlob(id));
	QVERIFY(!service.blob(id).has_value());
}

void TestSystemRouterCommandsDb::blobsCommand() {
	DataBlobsProviderDb provider(_databases.get(), QStringLiteral("system_router"));

	DataBlob first;
	first.id = kCommandId;
	first.version = 2;
	first.description = QStringLiteral("authorize");
	first.data = QStringLiteral("echo ok");
	QVERIFY(provider.saveBlob(first));

	DataBlob second;
	second.id = QStringLiteral("55555555-5555-5555-5555-555555555555");
	second.description = QStringLiteral("context");
	second.data = QStringLiteral("some data");
	QVERIFY(provider.saveBlob(second));

	auto service = std::make_unique<SystemRouterBlobService>(&provider);
	auto* blobService = service.get();
	_controller->testServices().add(std::move(service));

	QCOMPARE(blobService->blobs().size(), 2);

	QSignalSpy spy(_controller.get(), &CommandController::outputRequested);
	auto context = _controller->globalContext()->createScopedContext(QStringLiteral("blobs"));
	SystemRouterBlobsCommand command;
	QVERIFY(command.execute(context));
	QTRY_VERIFY(!spy.isEmpty());

	const auto output = spy.takeLast();
	QCOMPARE(output.at(2).toString(), QString::fromLatin1(kCommandPrintStyle_Table));
	const auto html = output.at(0).toString();
	QVERIFY(html.contains(kCommandId));
	QVERIFY(html.contains(QStringLiteral("authorize")));
	QVERIFY(html.contains(QStringLiteral("echo ok")));
}

void TestSystemRouterCommandsDb::routesCommand() {
	SystemRouterDataProviderDb provider(_databases.get());
	auto service = std::make_unique<SystemRouterService>(&provider);
	auto* routerService = service.get();
	_controller->testServices().add(std::move(service));

	const auto root = routerService->createItem({}, QStringLiteral("root"));
	QVERIFY(root);
	const auto child = routerService->createItem(root->id(), QStringLiteral("child"),
		{ { QStringLiteral("kind"), QStringLiteral("test") } });
	QVERIFY(child);

	QCOMPARE(routerService->allNodes().size(), 2);

	QSignalSpy spy(_controller.get(), &CommandController::outputRequested);
	auto context = _controller->globalContext()->createScopedContext(QStringLiteral("routes"));
	SystemRouterRoutesCommand command;
	QVERIFY(command.execute(context));
	QTRY_VERIFY(!spy.isEmpty());

	const auto output = spy.takeLast();
	QCOMPARE(output.at(2).toString(), QString::fromLatin1(kCommandPrintStyle_Table));
	const auto html = output.at(0).toString();
	QVERIFY(html.contains(QStringLiteral("root")));
	QVERIFY(html.contains(QStringLiteral("child")));
	QVERIFY(html.contains(QStringLiteral("kind")));
}

void TestSystemRouterCommandsDb::saveBlobCommand() {
	DataBlobsProviderDb provider(_databases.get(), QStringLiteral("system_router"));
	auto service = std::make_unique<SystemRouterBlobService>(&provider);
	auto* blobService = service.get();
	_controller->testServices().add(std::move(service));

	auto context = _controller->globalContext()->createScopedContext(QStringLiteral("save"));
	context->setVariable(QStringLiteral("blobId"), kCommandId);
	context->setVariable(QStringLiteral("data"), QStringLiteral("command script"));

	SystemRouterSaveBlobCommand command;
	QVERIFY(command.execute(context));
	QCOMPARE(blobService->data(kCommandId), QStringLiteral("command script"));
}

QTEST_MAIN(TestSystemRouterCommandsDb)
#include "test_system_router_commands_db.moc"
