#include "test_command.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_context_builder.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <memory>
#include <stdexcept>

namespace {
class TestCommandController : public CommandController {
public:
	TestCommandController() : CommandController(nullptr, nullptr) {
	}
	~TestCommandController() override = default;
	virtual CommandProcessor* commandProcessor() const override {
		return nullptr;
	}
	virtual ServicesRegistry* services() const {
		return nullptr;
	}

private:
	void initContext() override {
	}
};
}

void TestEngine::testContext_BuildChildContext() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto glob = ctl->globalContext();

	// When
	auto ch11 = glob->createScopedContext("test1");
	auto ch111 = ch11->createScopedContext("test11");
	auto ch12 = glob->createScopedContext("test2");

	auto ch11_ptr = glob->context(".test1");
	auto ch111_ptr = glob->context(".test1.test11");
	auto ch12_ptr = glob->context(".test2");

	// Then
	QCOMPARE(glob->name(), "");
	QCOMPARE(glob->nameSpace(), ".");
	QCOMPARE(ch11->name(), "test1");
	QCOMPARE(ch11->nameSpace(), ".test1");
	QCOMPARE(ch12->name(), "test2");
	QCOMPARE(ch12->nameSpace(), ".test2");
	QCOMPARE(ch111->name(), "test11");
	QCOMPARE(ch111->nameSpace(), ".test1.test11");
	QCOMPARE(ch11_ptr.get(), ch11.get());
	QCOMPARE(ch111_ptr.get(), ch111.get());
	QCOMPARE(ch12_ptr.get(), ch12.get());
	QCOMPARE(ch11_ptr->name(), "test1");
	QCOMPARE(ch11_ptr->nameSpace(), ".test1");
	QCOMPARE(ch12_ptr->name(), "test2");
	QCOMPARE(ch12_ptr->nameSpace(), ".test2");
	QCOMPARE(ch111_ptr->name(), "test11");
	QCOMPARE(ch111_ptr->nameSpace(), ".test1.test11");
}

void TestEngine::testContext_VariablesLazyMode() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto parent = ctl->globalContext()->createScopedContext("parent");
	parent->setVariable("token", "base");

	auto child = parent->createScopedContext("child");

	// When: до записи читаем значение базового контекста
	QCOMPARE(child->variable("token").toString(), QString("base"));

	// When: запись создаёт собственную копию
	QVERIFY(child->setVariable("token", "child"));

	// Then
	QCOMPARE(child->variable("token").toString(), QString("child"));
	QCOMPARE(parent->variable("token").toString(), QString("base"));
}

void TestEngine::testContext_VariablesMutableMode() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto parent = ctl->globalContext()->createScopedContext("parent");
	parent->setVariable("token", "base");

	auto child = parent->createScopedContext("child");
	child->setVariablesMode(CommandContext::VariablesMode::Mutable);

	// When
	QVERIFY(child->setVariable("token", "shared"));

	// Then: переменная общая с базовым контекстом
	QCOMPARE(child->variable("token").toString(), QString("shared"));
	QCOMPARE(parent->variable("token").toString(), QString("shared"));
}

void TestEngine::testContext_VariablesReadOnlyMode() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto parent = ctl->globalContext()->createScopedContext("parent");

	auto child = parent->createScopedContext("child");
	child->setVariablesMode(CommandContext::VariablesMode::ReadOnly);

	// When: запись запрещена
	QCOMPARE(child->setVariable("token", "forbidden"), false);

	// Then: чтение базового значения доступно
	parent->setVariable("token", "base");
	QCOMPARE(child->variable("token").toString(), QString("base"));
	QCOMPARE(child->hasVariable("token"), true);
}

void TestEngine::testContext_VariablesAtomicMode() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto parent = ctl->globalContext()->createScopedContext("parent");
	parent->setVariable("counter", 1);

	auto child = parent->createScopedContext("child");
	child->setVariablesMode(CommandContext::VariablesMode::Atomic);

	// When: изменение базового контекста на копию не влияет
	parent->setVariable("counter", 2);

	// Then
	QCOMPARE(child->variable("counter").toInt(), 1);

	// When
	QVERIFY(child->setVariable("counter", 3));

	// Then: и запись в копию не меняет базовый контекст
	QCOMPARE(child->variable("counter").toInt(), 3);
	QCOMPARE(parent->variable("counter").toInt(), 2);
}

void TestEngine::testContext_ScopeIsolation() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto parent = ctl->globalContext()->createScopedContext("parent");
	auto child = parent->createScopedContext("child");

	// When
	QVERIFY(child->setVariable("local", "value"));

	// Then: собственные переменные не видны базовому контексту
	QCOMPARE(child->variable("local").toString(), QString("value"));
	QCOMPARE(parent->hasVariable("local"), false);
	QCOMPARE(parent->variable("local", "none").toString(), QString("none"));
}

void TestEngine::testContext_BootstrapContextBuilder() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();

	BootstrapConfig::Config config;
	BootstrapConfig::Store store;
	store.name = "fetch_api";
	store.wal.checkInterval = 1234;
	store.wal.maxSizeMb = 7;
	config.stores.push_back(store);

	BootstrapContextBuilder builder(ctl.get());

	// When
	builder.reload(config);

	// Then
	auto context = ctl->globalContext()->context(".store.fetch_api");
	QVERIFY(context != nullptr);
	QCOMPARE(context->variable("wal.checkInterval").toInt(), 1234);
	QCOMPARE(context->variable("wal.maxSizeMb").toInt(), 7);
	QCOMPARE(context->variablesMode(), CommandContext::VariablesMode::ReadOnly);

	// When: конфигурация хранилищ не подписана на запись
	QCOMPARE(context->setVariable("wal.maxSizeMb", 99), false);

	// When: повторная загрузка не пересоздаёт контекст
	auto contextBefore = context.get();
	builder.reload(config);
	auto contextAfter = ctl->globalContext()->context(".store.fetch_api");
	QVERIFY(contextAfter != nullptr);
	QCOMPARE(contextAfter.get(), contextBefore);

	// Then: значения обновлены
	QCOMPARE(contextAfter->variable("wal.checkInterval").toInt(), 1234);
}

void TestEngine::testContext_ChildContexts() {
	auto ctl = std::make_unique<TestCommandController>();
	auto root = ctl->globalContext();
	auto parent = root->createScopedContext("parent");
	auto child = parent->createScopedContext("child");
	auto grandchild = child->createScopedContext("grandchild");
	QCOMPARE(root->childContexts().size(), 1);
	QCOMPARE(parent->childContexts().size(), 1);
	QCOMPARE(parent->childContexts().first().get(), child.get());
	QCOMPARE(child->childContexts().first().get(), grandchild.get());
	grandchild.reset();
	QVERIFY(child->childContexts().isEmpty());
}

namespace {
bool writeTestFile(const QString& path, const QByteArray& data) {
	QFile file(path);
	return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
}
}

void TestEngine::testContext_BootstrapMigrations() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	QVERIFY(QDir(dir.path()).mkdir("sql"));
	QVERIFY(writeTestFile(dir.filePath("sql/20251113181900-up-Initialize-database.sql"), "CREATE TABLE first(id INTEGER);"));
	QVERIFY(writeTestFile(dir.filePath("sql/20251113181900-down.sql"), "DROP TABLE first;"));
	QVERIFY(writeTestFile(dir.filePath("sql/20261113181900-up.sql"), "CREATE TABLE second(id INTEGER);"));
	QVERIFY(writeTestFile(dir.filePath("sql/20261113181900-down.sql"), "DROP TABLE second;"));
	// Неуказанный файл не должен загружаться.
	QVERIFY(writeTestFile(dir.filePath("sql/20271113181900-up.sql"), "unexpected"));
	const QByteArray yaml = R"(appearance:
  application:
    name: base
    viewport: {xtype: Test}
stores:
  items:
    accounts:
      path: data/accounts.db
      encrypted: true
      wal: {checkInterval: 1234, maxSizeMb: 7}
      migrations:
        - {version: 20261113181900, up: sql/20261113181900-up.sql, down: sql/20261113181900-down.sql}
        - {version: 20251113181900, description: Initialize, up: sql/20251113181900-up-Initialize-database.sql, down: sql/20251113181900-down.sql}
    empty: {}
    MixedCase: {path: data/mixed.db}
)";
	QVERIFY(writeTestFile(dir.filePath("bootstrap.yaml"), yaml));
	const auto config = BootstrapConfig::loadBootstrap(dir.filePath("bootstrap.yaml"));
	QCOMPARE(config.themeName, QString("dark"));
	QCOMPARE(config.stores.size(), size_t(3));
	auto ctl = std::make_unique<TestCommandController>();
	BootstrapContextBuilder builder(ctl.get());
	builder.reload(config);
	auto account = ctl->globalContext()->context(".store.accounts");
	QVERIFY(account);
	QCOMPARE(account->variable("path").toString(), QString("data/accounts.db"));
	QVERIFY(account->variable("encrypted").toBool());
	QCOMPARE(account->variable("wal.checkInterval").toInt(), 1234);
	QCOMPARE(account->variable("wal.maxSizeMb").toInt(), 7);
	QCOMPARE(account->variablesMode(), CommandContext::VariablesMode::ReadOnly);
	const auto migrations = account->variable("migrations").toList();
	QCOMPARE(migrations.size(), 2);
	const auto first = migrations.first().toMap();
	QCOMPARE(first.value("version").toLongLong(), qint64(20251113181900));
	QCOMPARE(first.value("description").toString(), QString("Initialize"));
	QCOMPARE(first.value("up").toString(), QString("CREATE TABLE first(id INTEGER);"));
	QCOMPARE(first.value("down").toString(), QString("DROP TABLE first;"));
	const auto second = migrations.last().toMap();
	QCOMPARE(second.value("version").toLongLong(), qint64(20261113181900));
	QCOMPARE(second.value("up").toString(), QString("CREATE TABLE second(id INTEGER);"));
	QCOMPARE(second.value("down").toString(), QString("DROP TABLE second;"));
	auto empty = ctl->globalContext()->context(".store.empty");
	QVERIFY(empty);
	QVERIFY(empty->variable("path").toString().isEmpty());
	QVERIFY(empty->variable("migrations").toList().isEmpty());
	QVERIFY(!empty->variable("encrypted").toBool());
	QCOMPARE(empty->variable("wal.checkInterval").toInt(), 60000);
	QCOMPARE(empty->variable("wal.maxSizeMb").toInt(), 20);
	// Псевдонимы хранилищ должны совпадать с нормализованными именами сервиса БД.
	auto mixed = ctl->globalContext()->context(".store.mixedcase");
	QVERIFY(mixed);
	QCOMPARE(mixed->variable("path").toString(), QString("data/mixed.db"));

	// Повторная загрузка обновляет прежний контекст, удаляя отсутствующее хранилище.
	BootstrapConfig::Config reloaded;
	BootstrapConfig::Store updated = config.stores.front();
	updated.migrations.clear();
	updated.path = "data/new.db";
	updated.encrypted = false;
	updated.wal.checkInterval = 4321;
	reloaded.stores.push_back(updated);
	empty.reset();
	builder.reload(reloaded);
	QCOMPARE(ctl->globalContext()->context(".store.accounts").get(), account.get());
	QVERIFY(!ctl->globalContext()->context(".store.empty"));
	QVERIFY(account->variable("migrations").toList().isEmpty());
	QCOMPARE(account->variable("path").toString(), QString("data/new.db"));
	QVERIFY(!account->variable("encrypted").toBool());
	QCOMPARE(account->variable("wal.checkInterval").toInt(), 4321);
	QCOMPARE(account->variablesMode(), CommandContext::VariablesMode::ReadOnly);
}

void TestEngine::testContext_BootstrapMigrationErrors() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	const QByteArray prefix = "appearance:\n  application:\n    name: base\n    viewport: {xtype: Test}\nstores:\n  items:\n    test:\n      migrations:\n";
	const auto path = dir.filePath("bootstrap.yaml");
	QVERIFY(writeTestFile(path, prefix + "        - {version: 20251113181900, up: 20251113181900-up.sql, down: 20251113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(dir.filePath("20251113181900-up.sql"), "up"));
	// Отсутствие отдельного down-файла также приводит к ошибке.
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(dir.filePath("20251113181900-down.sql"), "down"));
	QVERIFY(writeTestFile(path, prefix + "        - {version: 20251113181900, up: 20251113181900-up.sql, down: 20251113181900-down.sql}\n        - {version: 20251113181900, up: 20251113181900-up.sql, down: 20251113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(path, prefix + "        - {version: 20251113181900, up: 20251113181900-up.sql, down: 20261113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(path, prefix + "        - {version: 20251313181900, up: 20251313181900-up.sql, down: 20251313181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(path, prefix + "        - {version: 20251113181900, up: 20261113181900-up.sql, down: 20251113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(path, prefix + "        - {version: 20251113181900, up: 20251113181900-up.sql, down: 20251113181900-down-extra.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(path, prefix + "        - {version: 20251113181900, up: 20251113181900-up.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(path, prefix + "        - {version: 20251113181900, down: 20251113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
}
