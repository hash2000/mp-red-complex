#include "test_command.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_config.h"
#include "Libs/Engine/scripting/bootstrap/bootstrap_context_builder.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_context.h"

#include <memory>

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
