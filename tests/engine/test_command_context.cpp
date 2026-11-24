#include "test_command.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_context_builder.h"
#include "Libs/Engine/scripting/command_context/context_interceptor.h"
#include "Libs/Engine/scripting/command_context/document_rule.h"
#include "Libs/Engine/scripting/command_context/i_context_interceptor.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/system_options/system_option.h"
#include "Libs/Engine/scripting/system_options/system_options_registry.h"
#include "Libs/Resources/resources.h"

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
	explicit TestCommandController(Resources* resources) : CommandController(resources, nullptr) {
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

/// Правила, запрещающие запись и события: проверяем приоритет над VariablesMode.
class ForbidWriteInterceptor final : public StandardContextInterceptor {
public:
	bool canSetVariable(const CommandContext& context, const QString& name,
		const QVariant& value) const override {
		Q_UNUSED(context);
		Q_UNUSED(name);
		Q_UNUSED(value);
		return false;
	}

	bool canFireEvent(const CommandContext& context, const QString& eventName) const override {
		Q_UNUSED(context);
		Q_UNUSED(eventName);
		return false;
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

void TestEngine::testContext_PruneDeadContexts() {
	// Given: живые и уже уничтоженные вложенные контексты
	auto ctl = std::make_unique<TestCommandController>();
	auto glob = ctl->globalContext();

	auto alive = glob->createScopedContext("alive");
	auto aliveChild = alive->createScopedContext("child");

	{
		auto dead = glob->createScopedContext("dead");
		auto deadChild = dead->createScopedContext("child");
		auto deadGrandChild = deadChild->createScopedContext("grand");
		Q_UNUSED(deadGrandChild);
	}

	// Мёртвые записи видны только при явном запросе (alive = false).
	QVERIFY(glob->nameSpaces(false).contains(".dead"));
	QVERIFY(glob->nameSpaces(false).contains(".dead.child"));
	QVERIFY(glob->nameSpaces(false).contains(".dead.child.grand"));
	QVERIFY(glob->nameSpaces().contains(".alive"));
	// Для системы мёртвых контекстов не существует.
	QVERIFY(!glob->nameSpaces().contains(".dead"));

	// When
	const int removed = ctl->pruneDeadContexts();

	// Then: мёртвые ветки удалены, живые остались
	QCOMPARE(removed, 3);
	QVERIFY(!glob->nameSpaces(false).contains(".dead"));
	QVERIFY(!glob->nameSpaces(false).contains(".dead.child"));
	QVERIFY(!glob->nameSpaces(false).contains(".dead.child.grand"));
	QVERIFY(glob->nameSpaces().contains(".alive"));
	QVERIFY(glob->nameSpaces().contains(".alive.child"));
	QCOMPARE(glob->context(".alive").get(), alive.get());
	QCOMPARE(glob->context(".alive.child").get(), aliveChild.get());

	// Повторный вызов ничего не находит.
	QCOMPARE(ctl->pruneDeadContexts(), 0);
}

void TestEngine::testContext_NameSpacesAliveOnly() {
	// Given: живые и уничтоженные контексты на двух уровнях вложенности
	auto ctl = std::make_unique<TestCommandController>();
	auto glob = ctl->globalContext();

	auto alive = glob->createScopedContext("alive");
	auto aliveChild = alive->createScopedContext("child");

	{
		auto dead = glob->createScopedContext("dead");
		auto deadChild = dead->createScopedContext("child");
		Q_UNUSED(deadChild);
	}

	// When: по умолчанию и с alive = true
	const auto aliveOnly = glob->nameSpaces();

	// Then: мёртвых нет
	QVERIFY(aliveOnly.contains(".alive"));
	QVERIFY(aliveOnly.contains(".alive.child"));
	QVERIFY(!aliveOnly.contains(".dead"));
	QVERIFY(!aliveOnly.contains(".dead.child"));
	QCOMPARE(glob->nameSpaces(true), aliveOnly);

	// When: alive = false — реестр отдаёт и мёртвые записи
	const auto all = glob->nameSpaces(false);

	// Then
	QVERIFY(all.contains(".alive"));
	QVERIFY(all.contains(".alive.child"));
	QVERIFY(all.contains(".dead"));
	QVERIFY(all.contains(".dead.child"));
	QVERIFY(all.size() > aliveOnly.size());

	// Вложенный контекст отдаёт тот же список, что и глобальный.
	QCOMPARE(alive->nameSpaces(), aliveOnly);
	QCOMPARE(alive->nameSpaces(false), all);

	// После очистки оба списка совпадают: мёртвых записей не осталось.
	ctl->pruneDeadContexts();
	QCOMPARE(glob->nameSpaces(false), aliveOnly);
}

void TestEngine::testContext_ChildContextsReturnOnlyAlive() {
	// Given: у родителя два ребёнка, один из них уничтожается
	auto ctl = std::make_unique<TestCommandController>();
	auto glob = ctl->globalContext();

	auto parent = glob->createScopedContext("parent");
	auto live = parent->createScopedContext("live");

	{
		auto dead = parent->createScopedContext("dead");
		auto deadChild = dead->createScopedContext("grand");
		Q_UNUSED(deadChild);
	}

	// When
	const auto children = parent->childContexts();

	// Then: только живой ребёнок, мёртвый не возвращается
	QCOMPARE(children.size(), 1);
	QCOMPARE(children.front().get(), live.get());

	// У глобального контекста ребёнок — только parent.
	const auto globalChildren = glob->childContexts();
	QCOMPARE(globalChildren.size(), 1);
	QCOMPARE(globalChildren.front().get(), parent.get());

	// Прямые дети вызываются по имени только среди живых.
	QVERIFY(parent->context(".parent.dead") == nullptr);
	QCOMPARE(parent->context(".parent.live").get(), live.get());
}

void TestEngine::testContext_CleanupRunsInBackground() {
	// Given: контроллер с коротким периодом очистки
	Resources resources;
	resources.Variables.set("command.context.cleanupIntervalMs", 50);

	auto ctl = std::make_unique<TestCommandController>(&resources);
	ctl->startContextCleanup();

	{
		auto dead = ctl->globalContext()->createScopedContext("dead");
		auto deadChild = dead->createScopedContext("child");
		Q_UNUSED(deadChild);
	}
	QVERIFY(ctl->globalContext()->nameSpaces(false).contains(".dead.child"));

	// When/Then: фоновый поток сам вычищает мёртвую ветку из реестра
	QTRY_VERIFY_WITH_TIMEOUT(!ctl->globalContext()->nameSpaces(false).contains(".dead"), 5000);

	ctl->stopContextCleanup();
}

void TestEngine::testContext_EventIsFired() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto context = ctl->globalContext()->createScopedContext("events");
	QSignalSpy fired(context.get(), &CommandContext::eventFired);

	// When
	context->fireEvent("documents.changed");

	// Then: событие пришло подписчикам с именем в параметре
	QCOMPARE(fired.count(), 1);
	QCOMPARE(fired.at(0).at(0).toString(), QStringLiteral("documents.changed"));
}

void TestEngine::testContext_VariablesBatchSuppressesChangeSignals() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto context = ctl->globalContext()->createScopedContext("batch");
	QSignalSpy changed(context.get(), &CommandContext::variableChanged);
	QSignalSpy fired(context.get(), &CommandContext::eventFired);

	// When: пакет объявлен, переменные меняются
	context->beginVariablesBatch();
	QVERIFY(context->isVariablesBatchActive());
	context->setVariable("alpha", 1);
	context->setVariable("beta", 2);
	context->setVariable("alpha", 3);   // повторно — в списке один раз

	// Then: сигналов на отдельные переменные нет, список накоплен
	QCOMPARE(changed.count(), 0);
	QCOMPARE(context->changedVariables(), QStringList({ "alpha", "beta" }));

	// When: пакет завершён
	context->endVariablesBatch();

	// Then: пришло одно событие завершения, список очищен
	QVERIFY(!context->isVariablesBatchActive());
	QCOMPARE(fired.count(), 1);
	QCOMPARE(fired.at(0).at(0).toString(), CommandContext::variablesBatchCompleteEvent());
	QCOMPARE(fired.at(0).at(0).toString(), QStringLiteral("variables.batch.complete"));
	QVERIFY(context->changedVariables().isEmpty());

	// Изменения вне пакета снова рассылаются как обычно
	context->setVariable("gamma", 4);
	QCOMPARE(changed.count(), 1);
	QCOMPARE(changed.at(0).at(1).toString(), QStringLiteral("gamma"));
}

void TestEngine::testContext_VariablesBatchWithoutChanges() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto context = ctl->globalContext()->createScopedContext("empty-batch");
	QSignalSpy fired(context.get(), &CommandContext::eventFired);

	// When: пакет без изменений
	context->beginVariablesBatch();
	QVERIFY(context->changedVariables().isEmpty());
	context->endVariablesBatch();

	// Then: событие всё равно отправляется
	QCOMPARE(fired.count(), 1);
	QCOMPARE(fired.at(0).at(0).toString(), CommandContext::variablesBatchCompleteEvent());
}

void TestEngine::testContext_ChangedVariablesEmptyOutsideBatch() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto context = ctl->globalContext()->createScopedContext("outside");

	// Then: без объявленного пакета список пуст
	QVERIFY(!context->isVariablesBatchActive());
	QVERIFY(context->changedVariables().isEmpty());

	context->setVariable("alpha", 1);
	QVERIFY(context->changedVariables().isEmpty());

	// Завершение без начала ничего не делает
	QSignalSpy fired(context.get(), &CommandContext::eventFired);
	context->endVariablesBatch();
	QCOMPARE(fired.count(), 0);
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
	QVERIFY(QDir(dir.path()).mkdir("stores"));
	QVERIFY(writeTestFile(dir.filePath("sql/20251113181900-up-Initialize-database.sql"), "CREATE TABLE first(id INTEGER);"));
	QVERIFY(writeTestFile(dir.filePath("sql/20251113181900-down.sql"), "DROP TABLE first;"));
	QVERIFY(writeTestFile(dir.filePath("sql/20261113181900-up.sql"), "CREATE TABLE second(id INTEGER);"));
	QVERIFY(writeTestFile(dir.filePath("sql/20261113181900-down.sql"), "DROP TABLE second;"));
	// Неуказанный файл не должен загружаться.
	QVERIFY(writeTestFile(dir.filePath("sql/20271113181900-up.sql"), "unexpected"));
	// В bootstrap.yaml — только перечисление имён, настройки — в stores/<имя>.yaml.
	const QByteArray yaml = R"(appearance:
  application:
    name: base
    viewport: {xtype: Test}
stores:
  items:
    - accounts
    - empty
    - MixedCase
)";
	QVERIFY(writeTestFile(dir.filePath("bootstrap.yaml"), yaml));
	QVERIFY(writeTestFile(dir.filePath("stores/accounts.yaml"), R"(name: accounts
path: data/accounts.db
encrypted: true
wal: {checkInterval: 1234, maxSizeMb: 7}
migrations:
  - {version: 20261113181900, up: sql/20261113181900-up.sql, down: sql/20261113181900-down.sql}
  - {version: 20251113181900, description: Initialize, up: sql/20251113181900-up-Initialize-database.sql, down: sql/20251113181900-down.sql}
)"));
	QVERIFY(writeTestFile(dir.filePath("stores/empty.yaml"), "{}"));
	// Имя из списка нормализуется к нижнему регистру — так же называется файл.
	QVERIFY(writeTestFile(dir.filePath("stores/mixedcase.yaml"), "path: data/mixed.db\n"));
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
	QVERIFY(QDir(dir.path()).mkdir("stores"));
	const auto path = dir.filePath("bootstrap.yaml");
	const auto storePath = dir.filePath("stores/test.yaml");
	const QByteArray bootstrap = "appearance:\n  application:\n    name: base\n    viewport: {xtype: Test}\nstores:\n  items:\n    - test\n";
	QVERIFY(writeTestFile(path, bootstrap));

	const auto writeMigrations = [&storePath](const QByteArray& migrations) {
		return writeTestFile(storePath, "migrations:\n" + migrations);
	};

	QVERIFY(writeMigrations("  - {version: 20251113181900, up: 20251113181900-up.sql, down: 20251113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(dir.filePath("20251113181900-up.sql"), "up"));
	// Отсутствие отдельного down-файла также приводит к ошибке.
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeTestFile(dir.filePath("20251113181900-down.sql"), "down"));
	QVERIFY(writeMigrations("  - {version: 20251113181900, up: 20251113181900-up.sql, down: 20251113181900-down.sql}\n"
		"  - {version: 20251113181900, up: 20251113181900-up.sql, down: 20251113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeMigrations("  - {version: 20251113181900, up: 20251113181900-up.sql, down: 20261113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeMigrations("  - {version: 20251313181900, up: 20251313181900-up.sql, down: 20251313181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeMigrations("  - {version: 20251113181900, up: 20261113181900-up.sql, down: 20251113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeMigrations("  - {version: 20251113181900, up: 20251113181900-up.sql, down: 20251113181900-down-extra.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeMigrations("  - {version: 20251113181900, up: 20251113181900-up.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
	QVERIFY(writeMigrations("  - {version: 20251113181900, down: 20251113181900-down.sql}\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);
}

void TestEngine::testContext_BootstrapStoreFiles() {
	QTemporaryDir dir;
	QVERIFY(dir.isValid());
	QVERIFY(QDir(dir.path()).mkdir("stores"));
	const auto path = dir.filePath("bootstrap.yaml");
	const QByteArray header = "appearance:\n  application:\n    name: base\n    viewport: {xtype: Test}\nstores:\n";

	const auto writeBootstrap = [&path, &header](const QByteArray& stores) {
		return writeTestFile(path, header + stores);
	};

	// 1. Хранилище перечислено, но файла настроек нет — ошибка.
	QVERIFY(writeBootstrap("  items:\n    - missing\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);

	// 2. Файл существует, но имя не перечислено: хранилище отключено,
	//    файл настроек не читается и не мешает запуску.
	QVERIFY(writeTestFile(dir.filePath("stores/disabled.yaml"), "path: data/disabled.db\n"));
	QVERIFY(writeBootstrap("  items: []\n"));
	const auto disabled = BootstrapConfig::loadBootstrap(path);
	QVERIFY(disabled.stores.empty());

	// 3. Старый формат (map с настройками в bootstrap) отклоняется.
	QVERIFY(writeBootstrap("  items:\n    disabled:\n      path: data/disabled.db\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);

	// 4. Дубликаты имён без учёта регистра.
	QVERIFY(writeTestFile(dir.filePath("stores/dup.yaml"), "{}"));
	QVERIFY(writeBootstrap("  items:\n    - dup\n    - DUP\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);

	// 5. Недопустимое имя хранилища.
	QVERIFY(writeBootstrap("  items:\n    - ../escape\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);

	// 6. Имя в файле не совпадает с именем файла.
	QVERIFY(writeTestFile(dir.filePath("stores/named.yaml"), "name: other\n"));
	QVERIFY(writeBootstrap("  items:\n    - named\n"));
	QVERIFY_EXCEPTION_THROWN(BootstrapConfig::loadBootstrap(path), std::runtime_error);

	// 7. Пустой файл настроек — значения по умолчанию.
	QVERIFY(writeTestFile(dir.filePath("stores/plain.yaml"), "# только комментарий\n"));
	QVERIFY(writeBootstrap("  items:\n    - plain\n"));
	const auto plain = BootstrapConfig::loadBootstrap(path);
	QCOMPARE(plain.stores.size(), size_t(1));
	QCOMPARE(plain.stores.front().name, QString("plain"));
	QVERIFY(plain.stores.front().path.isEmpty());
	QVERIFY(!plain.stores.front().encrypted);
	QCOMPARE(plain.stores.front().wal.checkInterval, 60000);
	QCOMPARE(plain.stores.front().wal.maxSizeMb, 20);
	QVERIFY(plain.stores.front().migrations.empty());
}

void TestEngine::testContext_ReferenceBootstrap() {
	const auto path = QStringLiteral(REDCOMPLEX_CONFIGURATIONS_DIR)
		+ QStringLiteral("/bootstrap.yaml");
	QVERIFY(QFile::exists(path));

	const auto config = BootstrapConfig::loadBootstrap(path);
	QCOMPARE(config.stores.size(), size_t(5));

	const auto findStore = [&config](const QString& name) -> const BootstrapConfig::Store* {
		for (const auto& store : config.stores) {
			if (store.name == name) {
				return &store;
			}
		}
		return nullptr;
	};

	// Настройки вынесены в stores/<имя>.yaml и читаются по списку stores.items.
	const auto* fetch = findStore(QStringLiteral("fetch_api"));
	QVERIFY(fetch);
	QCOMPARE(fetch->wal.checkInterval, 60000);
	QCOMPARE(fetch->wal.maxSizeMb, 40);
	QCOMPARE(fetch->migrations.size(), size_t(2));
	QVERIFY(!fetch->encrypted);

	const auto* messages = findStore(QStringLiteral("messages"));
	QVERIFY(messages);
	QVERIFY(messages->migrations.empty());

	const auto* router = findStore(QStringLiteral("system_router"));
	QVERIFY(router);
	QCOMPARE(router->migrations.size(), size_t(3));

	const auto* accounts = findStore(QStringLiteral("accounts"));
	QVERIFY(accounts);
	QCOMPARE(accounts->wal.checkInterval, 120000);
	QCOMPARE(accounts->migrations.size(), size_t(1));

	const auto* game = findStore(QStringLiteral("game"));
	QVERIFY(game);
	QVERIFY(game->encrypted);
	QCOMPARE(game->migrations.size(), size_t(1));
}

void TestEngine::testContext_RemoveVariable() {
	// Given: собственные переменные контекста
	auto ctl = std::make_unique<TestCommandController>();
	auto context = ctl->globalContext()->createScopedContext("vars");
	QVERIFY(context->setVariable("alpha", 1));
	QVERIFY(context->setVariable("beta", 2));
	QSignalSpy changed(context.get(), &CommandContext::variableChanged);

	// When: удаляется собственная переменная
	QVERIFY(context->removeVariable("alpha"));

	// Then: значение пропало, подписчики получили сигнал об изменении
	QVERIFY(!context->hasVariable("alpha"));
	QCOMPARE(context->variable("alpha", "none").toString(), QString("none"));
	QCOMPARE(changed.count(), 1);
	QCOMPARE(changed.at(0).at(1).toString(), QString("alpha"));
	QCOMPARE(changed.at(0).at(2).toInt(), 1);
	QVERIFY(!changed.at(0).at(3).isValid());
	QCOMPARE(context->variable("beta").toInt(), 2);

	// Повторное удаление ничего не делает.
	QVERIFY(!context->removeVariable("alpha"));
	QCOMPARE(changed.count(), 1);

	// Унаследованную переменную удалить нельзя: она принадлежит базе.
	QVERIFY(context->parentContext()->setVariable("shared", "base"));
	QVERIFY(!context->removeVariable("shared"));
	QCOMPARE(context->variable("shared").toString(), QString("base"));
}

void TestEngine::testContext_DocumentRuleDocument() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	auto global = ctl->globalContext();
	auto documents = global->documentsContext();

	// When: обычный документ создаётся по роли DocumentRule::Document
	auto document = global->createDocumentContext("notes");

	// Then: текущее поведение — контекст `.documents.notes`, режим Lazy
	QVERIFY(document);
	QCOMPARE(document->nameSpace(), QString(".documents.notes"));
	QCOMPARE(document->variablesMode(), CommandContext::VariablesMode::Lazy);
	QCOMPARE(document->documentId(), QString("notes"));
	QVERIFY(document->isDocumentContext());

	// Правила роли: документы, переменные и события разрешены.
	QVERIFY(document->interceptor());
	QCOMPARE(document->interceptor()->scopeName(), QString("documents"));
	QVERIFY(!document->interceptor()->documentLivesInScope());
	QVERIFY(!document->interceptor()->variablesMode().has_value());

	// Запись создаёт собственную переменную и не меняет контекст документов.
	QVERIFY(document->setVariable("title", "Заметки"));
	QCOMPARE(document->variable("title").toString(), QString("Заметки"));
	QVERIFY(!documents->ownVariables().contains("title"));
}

void TestEngine::testContext_DocumentsContextIsReadOnly() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	ctl->initSystemContexts();

	// When
	auto documents = ctl->documentsContext();

	// Then: документы не могут менять системные параметры через свой корень
	QVERIFY(documents);
	QCOMPARE(documents->variablesMode(), CommandContext::VariablesMode::ReadOnly);
	QCOMPARE(documents->setVariable(SystemOptions::kDocumentsTabsOrder, "color"), false);
	QCOMPARE(ctl->globalContext()->variable(SystemOptions::kDocumentsTabsOrder).toString(),
		QString("manual"));
}

void TestEngine::testContext_SystemOptionsDocument() {
	// Given: системные параметры записаны в глобальный контекст
	auto ctl = std::make_unique<TestCommandController>();
	ctl->initSystemContexts();
	auto global = ctl->globalContext();
	QCOMPARE(global->variable(SystemOptions::kDocumentsTabsOrder).toString(), QString("manual"));

	// When: создаётся документ системных настроек
	auto document = global->createDocumentContext("system-options", DocumentRule::SystemOptions);

	// Then: он живёт прямо в контексте `.options` и работает в режиме Mutable
	QVERIFY(document);
	QCOMPARE(document->nameSpace(), QString(".options"));
	QCOMPARE(document->variablesMode(), CommandContext::VariablesMode::Mutable);
	QVERIFY(document->interceptor());
	QCOMPARE(document->interceptor()->scopeName(), QString("options"));
	QVERIFY(document->interceptor()->documentLivesInScope());

	// Запись в режиме Mutable попадает в глобальный контекст.
	QVERIFY(document->setVariable(SystemOptions::kDocumentsTabsOrder, "color"));
	QCOMPARE(global->variable(SystemOptions::kDocumentsTabsOrder).toString(), QString("color"));

	// Повторный вызов возвращает тот же контекст (`.options` — один на приложение).
	QCOMPARE(global->createDocumentContext("system-options", DocumentRule::SystemOptions).get(),
		document.get());

	// Обычный документ менять настройку не может: запись остаётся локальной.
	auto plain = global->documentsContext()->createDocumentContext("plain");
	QVERIFY(plain);
	QVERIFY(plain->setVariable(SystemOptions::kDocumentsTabsOrder, "xtype"));
	QCOMPARE(global->variable(SystemOptions::kDocumentsTabsOrder).toString(), QString("color"));
}

void TestEngine::testContext_InterceptorPriorityOverVariablesMode() {
	// Given: правила контекста запрещают запись и события
	auto ctl = std::make_unique<TestCommandController>();
	auto context = ctl->globalContext()->createScopedContext("guarded");
	context->setInterceptor(std::make_unique<ForbidWriteInterceptor>());

	// When: режим переменных разрешает запись
	context->setVariablesMode(CommandContext::VariablesMode::Mutable);
	QSignalSpy fired(context.get(), &CommandContext::eventFired);

	// Then: правила приоритетнее режима — запись и событие отклонены
	QCOMPARE(context->setVariable("token", "value"), false);
	QVERIFY(!context->hasVariable("token"));
	context->fireEvent("documents.changed");
	QCOMPARE(fired.count(), 0);

	// Сброс правил возвращает стандартное поведение.
	context->setInterceptor(nullptr);
	QVERIFY(context->setVariable("token", "value"));
	context->fireEvent("documents.changed");
	QCOMPARE(fired.count(), 1);
}

void TestEngine::testContext_SystemOptionsDescription() {
	// Given: встроенный системный параметр сортировки вкладок
	auto ctl = std::make_unique<TestCommandController>();
	auto* registry = ctl->systemOptions();
	QVERIFY(registry);

	// Then: параметр описан, значения совпадают с реестром правил
	const auto option = registry->option(SystemOptions::kDocumentsTabsOrder);
	QVERIFY(option);
	QCOMPARE(option->type, SystemOptionType::Enum);
	QCOMPARE(option->defaultValue.toString(), QString("manual"));
	QCOMPARE(option->values, QStringList({ "manual", "color", "xtype" }));
	QCOMPARE(option->valueTitles.size(), option->values.size());
	QVERIFY(!option->title.isEmpty());
	QVERIFY(registry->optionNames().contains(SystemOptions::kDocumentsTabsOrder));
	QVERIFY(!registry->option("missing.option").has_value());

	// When: значения по умолчанию записываются в глобальный контекст
	ctl->initSystemContexts();

	// Then: параметр появился, а повторное применение не перезаписывает значение
	QCOMPARE(ctl->globalContext()->variable(SystemOptions::kDocumentsTabsOrder).toString(),
		QString("manual"));
	QVERIFY(ctl->globalContext()->setVariable(SystemOptions::kDocumentsTabsOrder, "xtype"));
	QCOMPARE(ctl->systemOptions()->applyDefaults(*ctl->globalContext()), 0);
	QCOMPARE(ctl->globalContext()->variable(SystemOptions::kDocumentsTabsOrder).toString(),
		QString("xtype"));
}
