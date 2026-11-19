#include "test_command.h"
#include "Libs/Engine/scripting/command_context.h"
#include "Libs/Engine/scripting/command_controller.h"
#include "Libs/Engine/scripting/command_processor.h"
#include "Libs/Engine/scripting/i_command.h"
#include "Libs/Engine/scripting/parser/commands/parser_command.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <memory>
#include <optional>
#include <utility>

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

/// Команда-шпион: запоминает контекст, с которым её вызвали.
class SpyCommand : public ICommand {
public:
	explicit SpyCommand(QString name)
		: ICommand(nullptr)
		, _name(std::move(name)) {
	}

	bool execute(std::shared_ptr<CommandContext> context) override {
		executed = true;
		received = std::move(context);
		return true;
	}

	QString name() const override {
		return _name;
	}

	QString description() const override {
		return "Test command";
	}

	bool executed = false;
	std::shared_ptr<CommandContext> received;

private:
	QString _name;
};

} // namespace

void TestEngine::testParser_ParseCommand_ShouldReturnNameAndParameters() {
	// Given
	QString testScript = R"(windows action=create, target=fetch-api, id=42)";
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserCommand().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);
	QCOMPARE(result->parameter("name").toString(), QString("windows"));

	const auto parameters = result->children("parameter");
	QCOMPARE(parameters.size(), size_t(3));

	QCOMPARE(parameters[0]->parameter("name").toString(), QString("action"));
	QCOMPARE(parameters[0]->parameter("value").toString(), QString("create"));
	QCOMPARE(parameters[0]->parameter("mime-type").toString(), QString("text/plane"));

	QCOMPARE(parameters[1]->parameter("name").toString(), QString("target"));
	QCOMPARE(parameters[1]->parameter("value").toString(), QString("fetch-api"));

	QCOMPARE(parameters[2]->parameter("name").toString(), QString("id"));
	QCOMPARE(parameters[2]->parameter("value").toInt(), 42);
}

void TestEngine::testParser_ParseCommand_ShouldParseQuotedAndBlockValues() {
	// Given
	QString testScript = R"(echo title="Hello world", body=<<json>>{"items":[1,2]}<</json>>)";
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserCommand().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);
	QCOMPARE(result->parameter("name").toString(), QString("echo"));

	const auto parameters = result->children("parameter");
	QCOMPARE(parameters.size(), size_t(2));

	QCOMPARE(parameters[0]->parameter("value").toString(), QString("Hello world"));
	QCOMPARE(parameters[0]->parameter("mime-type").toString(), QString("text/plane"));

	const auto jsonMimeType = parameters[1]->parameter("mime-type").toString();
	QCOMPARE(jsonMimeType, QString("application/json"));

	const auto json = parameters[1]->parameter("value").toJsonDocument().object();
	QCOMPARE(json.value("items").toArray().size(), 2);
}

void TestEngine::testParser_ParseCommand_ShouldRejectLegacyBlockValues() {
	// Given: старый формат блока значения больше не поддерживается
	QString testScript = R"(echo body=<json>{"a":1}</json>)";
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserCommand().parse(source, error);

	// Then
	QVERIFY(result == nullptr);
	QCOMPARE(error.has_value(), true);
}

void TestEngine::testParser_ParseCommand_ShouldFailWithoutEquals() {
	// Given: старый синтаксис с двоеточием больше не поддерживается
	QString testScript = R"(echo action:create)";
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserCommand().parse(source, error);

	// Then
	QVERIFY(result == nullptr);
	QCOMPARE(error.has_value(), true);
}

void TestEngine::testParser_ParseCommand_ShouldRequireCommaSeparator() {
	// Given: параметры не разделены запятой
	QString testScript = R"(windows action=create target=fetch-api)";
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserCommand().parse(source, error);

	// Then
	QVERIFY(result == nullptr);
	QCOMPARE(error.has_value(), true);
}

void TestEngine::testCommandProcessor_Execute_CreatesMutableCommandContext() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	CommandProcessor processor(nullptr);

	auto command = std::make_unique<SpyCommand>("probe");
	auto* spy = command.get();
	processor.registerCommand(std::move(command));

	// When
	const auto result = processor.execute("probe action=create, id=42", ctl->globalContext());

	// Then
	QVERIFY(result);
	QVERIFY(spy->executed);
	QVERIFY(spy->received != nullptr);
	QCOMPARE(spy->received->name(), QString("probe"));
	QCOMPARE(spy->received->variablesMode(), CommandContext::VariablesMode::Mutable);
	QCOMPARE(spy->received->variable("action").toString(), QString("create"));
	QCOMPARE(spy->received->variable("id").toInt(), 42);
	QCOMPARE(spy->received->variableMimeType("action"), QString("text/plane"));
	QCOMPARE(spy->received->parentContext().get(), ctl->globalContext().get());
}

void TestEngine::testCommandProcessor_Execute_KeepsParentVariables() {
	// Given
	auto ctl = std::make_unique<TestCommandController>();
	CommandProcessor processor(nullptr);

	auto command = std::make_unique<SpyCommand>("probe");
	auto* spy = command.get();
	processor.registerCommand(std::move(command));

	ctl->globalContext()->setVariable("action", "base");

	// When: параметр команды совпадает по имени с переменной базового контекста
	const auto result = processor.execute("probe action=create", ctl->globalContext());

	// Then: параметр остался в контексте команды, базовый контекст не изменился
	QVERIFY(result);
	QCOMPARE(spy->received->variable("action").toString(), QString("create"));
	QCOMPARE(ctl->globalContext()->variable("action").toString(), QString("base"));
}
