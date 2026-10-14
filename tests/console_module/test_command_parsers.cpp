#include <QtTest/QtTest>

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
		string[secret,mime("text/plane")] key; // в объекте session объявить секретную переменную key (её значение не выводится никуда (лог, экран), но можно пользоваться нативно при использовании внутри ICommand). точка с запятой в конце означает , что переменная объявлена, но значения нет. если стоит двоеточик ':', то дальче читается значение по правилам ниже? точка с запятой в конце не нужна.
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
)";;
	QString _testInclude = R"(
	// комментарий

	#include "base/utils.script" // подключить файл
)";
	QString _testTypedVariableDeclaration = R"(
	// тип данных (string, int, double, bool)
	string[secret,mime("text/plane")] key;
)";

private slots:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	void testCommandParser_ParseInclude_ShouldReturnIncludeAstWithPath();
	void testCommandParser_ParseTypedVariable_ShouldReturnVariableDeclarationWithoutValue();
};
 
#include "Content/ConsoleModule/processors/parser/commands/parser_command.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_include.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_include.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_variable.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_variable.h"

void TestConsoleModule::initTestCase() {}
void TestConsoleModule::cleanupTestCase() {}

void TestConsoleModule::init() {
}

void TestConsoleModule::cleanup() {
}

void TestConsoleModule::testCommandParser_ParseTypedVariable_ShouldReturnVariableDeclarationWithoutValue() {
	// Prepare
	SourceCursor source(_testTypedVariableDeclaration);
	std::optional<ParseError> error;

	// Pass
	auto result = ParserVariable().parse(source, error);
	auto ast = static_cast<AstVariable*>(result.get());

	// Verify
	auto location = result->location();
	auto srcLocation = source.currentLocation();
	auto nextCh = source.peek();

	QVERIFY(result != nullptr);
	QVERIFY(ast != nullptr);
}

void TestConsoleModule::testCommandParser_ParseInclude_ShouldReturnIncludeAstWithPath() {
	// Prepare
	SourceCursor source(_testInclude);
	std::optional<ParseError> error;

	// Pass
	auto result = ParserInclude().parse(source, error);
	auto ast = static_cast<AstInclude*>(result.get());

	// Verify
	auto location = result->location();
	auto srcLocation = source.currentLocation();
	auto nextCh = source.peek();

	QVERIFY(result != nullptr);
	QVERIFY(ast != nullptr);
	QCOMPARE(ast->path, "base/utils.script");	
	QCOMPARE(error.has_value(), false);
	QCOMPARE(location.offset, 19);
	QCOMPARE(location.column, 2);
	QCOMPARE(location.line, 4);
	QCOMPARE(nextCh, " ");
	QCOMPARE(srcLocation.offset, 47);
	QCOMPARE(srcLocation.column, 30);
	QCOMPARE(srcLocation.line, 4);
}




QTEST_MAIN(TestConsoleModule)
#include "test_command_parsers.moc"
