#include "test_command.h"
#include "Content/ConsoleModule/command_controller.h"
#include "Content/ConsoleModule/command_context.h"

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

void TestConsoleModule::testCommandContext_BuildChildContext() {
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

