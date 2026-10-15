#include <QtTest/QtTest>
#include <set>

class TestConsoleModule : public QObject {
	Q_OBJECT

public:
	TestConsoleModule() = default;
	~TestConsoleModule() = default;

private:
	QString _testScript = R"(
#include "base/utils.script" // подключить файл

context .system.auth { // объявить контекст

	define session { // внутри текущего контекста объявить объект session
		string[secret, mime("text/plane")] key; // в объекте session объявить секретную переменную key (её значение не выводится никуда (лог, экран), но можно пользоваться нативно при использовании внутри ICommand). точка с запятой в конце означает , что переменная объявлена, но значения нет. если стоит двоеточик ':', то дальче читается значение по правилам ниже? точка с запятой в конце не нужна.
		// <<text>><</text>> - многострочный текст
		// "" - однострочный текст, с экранированными символами как в C++
		// <<json>><</json>> - формат json
		// <<xml>><</xml>> - формат xml
		string user; // переменная user 
		var description:
		// значение переменной читается от открывающегося тега и до закрывающегося у переменной mime тип "text/plane"	
		<<text>> 
			сессия пользователя
		<</text>>
		var json: 
		// значение переменной читается от открывающегося тега и до закрывающегося у переменной mime тип "application/json"
		// значение переменной хранится как 
		// QJsonDocument doc = QJsonDocument::fromJson(
		// QVariant::fromValue(doc)
		<<json>> 
			{
				"mime": "session",
				"controller": ".session.auth.SessionController"
			}
		<</json>>
		var userPage: 
		// значение переменной читается от открывающегося тега и до закрывающегося у переменной mime тип "application/xml"
		// значение переменной хранится как 
		// QDomDocument doc;
		// QVariant::fromValue(doc)
		<<xml>>
			<head></head>
			<body></body>
		<</xml>>
		var params:
		<<ini>>
		option1:value1
		option2:value2
		<</ini>>
	}

	// объявление переменных. на бэке будет вызван класс производный от ICommand который обработает переменные 
	var {
		session[secret] current;
		session[repeated, secret] previus;
		var label = "Authorization"
	}

	// выполнить компнду объявленную class WindowsCommand : public ICommand
	// параметры читаюся "имя параметра":"значение" заканчивается чтение параметров команды ;
	windows 
		action:create
		target:login
		desctiption:<text>
			Войти в систему
		</text>
		// результат поместить в контекст .globalContext.test.auth
		// если сразу стоит ; поместить в текущий контекст
		>> .globalContext.test.auth;	
}
)";

private slots:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();


	void testCommandParser_SourceCursorReadUntil_ShouldReturnTextBetweenQuotesMarkers();
	void testCommandParser_ParseAttributesCollection_ShouldReturnAttribetes();
	void testCommandParser_ParseInclude_ShouldReturnIncludeAstWithPath();
	void testCommandParser_ParseTypedVariable_ShouldReturnVariableDeclarationWithoutValue();
	void testCommandParser_ParseSingleTextValue_ShouldReturnAstValue();
	void testCommandParser_ParseMultilineTextValue_ShouldReturnAstValue();
};
 
#include "Content/ConsoleModule/processors/parser/commands/parser_command.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_include.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_variable.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_value.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_attributes_collection.h"

void TestConsoleModule::initTestCase() {}
void TestConsoleModule::cleanupTestCase() {}

void TestConsoleModule::init() {
}

void TestConsoleModule::cleanup() {
}

void TestConsoleModule::testCommandParser_ParseAttributesCollection_ShouldReturnAttribetes() {
	// Prepare
	QString script = R"(
	[ // секретный, нельзя просматривать в консоли или другими встроенными средствами отладки
		secret, // это секрет
		// mime-тип - text/plane
		testParametrizedAttribyte("param1", <<text>>
	parameter value
	<</text>>, "test"),
		// обозначает коллекцию 
		repeated
	]
)";
	SourceCursor source(script);
	std::optional<ParseError> error;

	// Pass
	auto result = ParserAttributesCollection().parse(source, error);

	// Verify
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

	QCOMPARE(result->children().size(), 3);

	std::set<QString> found;
	for (const auto attr : result->children()) {
		auto name = attr->parameter("name").toString();
		auto inserResult = found.emplace(name);
		QCOMPARE(inserResult.second, true);

		if (name == "secret") {
			QCOMPARE(attr->children().size(), 0);
		}
		else if (name == "testParametrizedAttribyte") {
			QCOMPARE(attr->children().size(), 3);
			int paramid = 0;
			for (const auto param : attr->children()) {
				auto paramValue = param->parameter("value").toString();
				auto paramMimeType = param->parameter("mime-type").toString();
				if (paramid == 0) {
					QCOMPARE(paramValue, "param1");
				}
				else if (paramid == 1) {
					QCOMPARE(paramValue, R"(
	parameter value
	)");
				}
				else if (paramid == 2) {
					QCOMPARE(paramValue, "test");
				}
				paramid++;
			}
		}
		else if (name == "repeated") {
			QCOMPARE(attr->children().size(), 0);
		}
	}

	QCOMPARE(found.size(), 3);
}

void TestConsoleModule::testCommandParser_ParseTypedVariable_ShouldReturnVariableDeclarationWithoutValue() {
	// Prepare
	QString testTypedVariableDeclaration = R"(
	// тип данных (string, int, double, bool)
	string[secret mime("text/plane")] key;
)";

	SourceCursor source(testTypedVariableDeclaration);
	std::optional<ParseError> error;

	// Pass
	auto result = ParserVariable().parse(source, error);

	// Verify
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

}

void TestConsoleModule::testCommandParser_ParseSingleTextValue_ShouldReturnAstValue() {
	// Prepare
	QString script = R"(
	// комментарий
	// комментарий
"тестовая строка"
)";
	SourceCursor source(script);
	std::optional<ParseError> error;

	// Pass
	auto result = ParserValue().parse(source, error);

	// Verify
	QVERIFY(result != nullptr);

	auto srcLoc = source.currentLocation();
	auto location = result->location();
	auto srcLocation = source.currentLocation();

	auto name = result->parameter("name");
	auto value = result->parameter("value");
	auto mime = result->parameter("mime-type");

	QCOMPARE_NE(source.peek(), '\"');
	QCOMPARE(location.offset, 33);
	QCOMPARE(srcLoc.offset, 50);
	QCOMPARE(name.isNull(), true);
	QCOMPARE(value.isNull(), false);
	QCOMPARE(value, "тестовая строка");
	QCOMPARE(mime, "text/plane");
}

void TestConsoleModule::testCommandParser_ParseMultilineTextValue_ShouldReturnAstValue() {
	// Prepare
	QString script = R"(
	// комментарий
	// комментарий
<<text>>
	1	тестовая строка
	2 текстовая строка
<</text>> 
)";
	SourceCursor source(script);
	std::optional<ParseError> error;

	// Pass
	auto result = ParserValue().parse(source, error);

	// Verify
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

	auto name = result->parameter("name");
	auto value = result->parameter("value").toString();
	auto mime = result->parameter("mime-type").toString();

	QCOMPARE(name.isNull(), true);
	QCOMPARE(value.isNull(), false);
	QCOMPARE(value, R"(
	1	тестовая строка
	2 текстовая строка
)");
	QCOMPARE(mime, "text/plane");
}

void TestConsoleModule::testCommandParser_ParseInclude_ShouldReturnIncludeAstWithPath() {
	// Prepare
	QString testInclude = R"(
	// комментарий

	#include "base/utils.script" // подключить файл
)";
	SourceCursor source(testInclude);
	std::optional<ParseError> error;

	// Pass
	auto result = ParserInclude().parse(source, error);

	// Verify
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

	QCOMPARE(result->parameter("path").isNull(), false);
	QCOMPARE(result->parameter("path").toString(), "base/utils.script");
	QCOMPARE(error.has_value(), false);
	QCOMPARE(location.offset, 19);
	QCOMPARE(location.column, 2);
	QCOMPARE(location.line, 4);
	QCOMPARE(source.peek(), " ");
	QCOMPARE(srcLocation.offset, 47);
	QCOMPARE(srcLocation.column, 30);
	QCOMPARE(srcLocation.line, 4);
}

void TestConsoleModule::testCommandParser_SourceCursorReadUntil_ShouldReturnTextBetweenQuotesMarkers() {
	// Prepare
	QString prefix = "any block ";
	QString suffif = R"(<<text>>
	1 тестовая строка
	2 текстовая строка
<</text>> 
)";
	QString script = QString("%1%2")
		.arg(prefix)
		.arg(suffif);

	SourceCursor source(script);
	std::optional<ParseError> error;

	// Pass
	source.advance(prefix.length());
	auto block = source.tryReadQuotedBlock("<<text>>", "<</text>>", true, "", error).toUtf8();

	// Verify
	QCOMPARE(block.isEmpty(), false);
	QCOMPARE(block.startsWith("<<text>>"), false);
	QCOMPARE(block.endsWith("<</text>>"), false);
	QCOMPARE(block, R"(
	1 тестовая строка
	2 текстовая строка
)");

}


QTEST_MAIN(TestConsoleModule)
#include "test_command_parsers.moc"
