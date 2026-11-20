#include "Content/DatabaseModule/services/databases_service.h"
#include "Content/SystemRouter/data_providers/system_router_commands_provider_db.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Resources/resources.h"

#include <QTemporaryDir>
#include <QtTest/QtTest>

#include <memory>

namespace {

constexpr qint64 kMigrationVersion = 20261120090000LL;
const QString kCommandId = QStringLiteral("33333333-3333-3333-3333-333333333333");

const QString kMigrationUp = QStringLiteral(R"(
CREATE TABLE "nodes" (
	"id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
	"parent_id" INTEGER,
	"name" TEXT NOT NULL,
	"data" TEXT NOT NULL DEFAULT '{}'
);
CREATE TABLE "commands" (
	"id" TEXT NOT NULL PRIMARY KEY,
	"version" INTEGER NOT NULL DEFAULT 1,
	"description" TEXT NOT NULL DEFAULT '',
	"script" TEXT NOT NULL DEFAULT ''
);
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

} // namespace

class TestSystemRouterCommandsDb : public QObject {
	Q_OBJECT
private slots:
	void init();
	void cleanup();

	void savesReadsAndUpdatesCommand();
	void removesCommand();

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
	_storeContext->setVariable(QStringLiteral("migrations"), QVariantList{ migration });

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

QTEST_MAIN(TestSystemRouterCommandsDb)
#include "test_system_router_commands_db.moc"
