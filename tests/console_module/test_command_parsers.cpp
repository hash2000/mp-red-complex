#include <QtTest/QtTest>
#include <set>

class TestConsoleModule : public QObject {
	Q_OBJECT

public:
	TestConsoleModule() = default;
	~TestConsoleModule() = default;

private slots:
	void initTestCase();
	void cleanupTestCase();
	void init();
	void cleanup();

	void testCommandParser_SourceCursorReadUntil_ShouldReturnTextBetweenQuotesMarkers();
	void testCommandParser_ParseAttributesCollection_ShouldReturnAttribetes();
	void testCommandParser_ParseInclude_ShouldReturnIncludeAstWithPath();
	void testCommandParser_ParseSingleTextValue_ShouldReturnAstValue();
	void testCommandParser_ParseMultilineTextValue_ShouldReturnAstValue();
	void testCommandParser_ParseUntypedVariableWithoutRValue_ShouldReturnError();
	void testCommandParser_ParseTypedVariable_ShouldReturnVariableDeclarationWithoutValue();
	void testCommandParser_ParseRValueTypedVariable_ShouldReturnVariableDeclarationWithValue();
	void testCommandParser_ParseVariableWithBinding_ShouldReturnVariableDeclarationWithBindName();
	void testCommandParser_ParseObject_ShouldReturnObjectWithFields();
	void testCommandParser_ParseBoolValue_ShouldReturnTrueFalseValue_data();
	void testCommandParser_ParseBoolValue_ShouldReturnTrueFalseValue();
	void testCommandParser_ParseNumberValue_ShouldReturnPositiveOrNegativeNumber_data();
	void testCommandParser_ParseNumberValue_ShouldReturnPositiveOrNegativeNumber();
};
 
#include "Content/ConsoleModule/processors/parser/commands/parser_command.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_include.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_variable.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_value.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_attributes_collection.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_object.h"

void TestConsoleModule::initTestCase() {}
void TestConsoleModule::cleanupTestCase() {}

void TestConsoleModule::init() {
}

void TestConsoleModule::cleanup() {
}

void TestConsoleModule::testCommandParser_ParseObject_ShouldReturnObjectWithFields() {
	// Given
	QString testScript = R"(
	object car {
		string vendor;
		string vin;
		number aaaa;
		bool isEnabled;
	}
)";

	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserObject().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

}

void TestConsoleModule::testCommandParser_ParseNumberValue_ShouldReturnPositiveOrNegativeNumber_data() {
	QTest::addColumn<QString>("testScript");
	QTest::addColumn<double>("expected");

	QTest::newRow("+456") << R"(
	// комментарий 
	+456
)" << 456.0;

	QTest::newRow("123") << R"(
	// комментарий 
	123
)" << 123.0;

	QTest::newRow("-789") << R"(
	// комментарий 
	-789
)" << -789.0;

	QTest::newRow("1_000") << R"(
	// комментарий 
	1_000
)" << 1000.0;

	QTest::newRow("1000") << R"(
	// комментарий 
	1000
)" << 1000.0;

	QTest::newRow("1_000_000") << R"(
	// комментарий 
	1_000_000
)" << 1000000.0;

	QTest::newRow("3.14") << R"(
	// комментарий 
	3.14
)" << 3.14;

	QTest::newRow("-0.5") << R"(
	// комментарий 
	-0.5
)" << -0.5;

	QTest::newRow("1_000.50") << R"(
	// комментарий 
	1_000.50
)" << 1000.5;

	QTest::newRow("+1_234.567") << R"(
	// комментарий 
	+1_234.567
)" << 1234.567;
}

void TestConsoleModule::testCommandParser_ParseNumberValue_ShouldReturnPositiveOrNegativeNumber() {
	// Given
	QFETCH(QString, testScript);
	QFETCH(double, expected);
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserValue().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

	QCOMPARE(result->parameter("value").isNull(), false);
	QCOMPARE(result->parameter("value").toDouble(), expected);
}


void TestConsoleModule::testCommandParser_ParseBoolValue_ShouldReturnTrueFalseValue_data() {
	QTest::addColumn<QString>("testScript");
	QTest::addColumn<bool>("expected");

	QTest::newRow("true value") << R"(
	// true value
	true
)" << true;

	QTest::newRow("false value") << R"(
	// false value
	false
)" << false;

}

void TestConsoleModule::testCommandParser_ParseBoolValue_ShouldReturnTrueFalseValue() {
	// Given
	QFETCH(QString, testScript);
	QFETCH(bool, expected);
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserValue().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

	QCOMPARE(result->parameter("value").isNull(), false);
	QCOMPARE(result->parameter("value").toBool(), expected);
}

void TestConsoleModule::testCommandParser_ParseUntypedVariableWithoutRValue_ShouldReturnError() {
	// Given
	QString testScript = R"(
	// тип данных (string, number, bool)
	var key;
)";

	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserVariable().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), true);
	QVERIFY(result == nullptr);
}

void TestConsoleModule::testCommandParser_ParseVariableWithBinding_ShouldReturnVariableDeclarationWithBindName() {
	// Given
	QString testScript = R"(
	// тип данных (string, number, bool)
	string key:{{anotherValue}};
)";

	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserVariable().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);
	QCOMPARE(result->parameter("name").toString(), "key");
	QCOMPARE(result->parameter("value").isNull(), true);
	QCOMPARE(result->parameter("bind").toString(), "anotherValue");
}

void TestConsoleModule::testCommandParser_ParseTypedVariable_ShouldReturnVariableDeclarationWithoutValue() {
	// Given
	QString testScript = R"(
	// тип данных (string, number, bool)
	string[secret, mime("text/plane"), rare] key;
)";

	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserVariable().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();
	QCOMPARE(result->parameter("name").toString(), "key");
	QCOMPARE(result->parameter("value").isNull(), true);
}

void TestConsoleModule::testCommandParser_ParseRValueTypedVariable_ShouldReturnVariableDeclarationWithValue() {
	// Given
	QString testScript = R"(
	// тип данных (string, number, bool)
	var[secret, rare] key: <<text>>super puper
multiline secret text
<</text>>;
)";

	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserVariable().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();
	QCOMPARE(result->parameter("name").toString(), "key");
	QCOMPARE(result->parameter("value").toString(), R"(super puper
multiline secret text
)");
	
}

void TestConsoleModule::testCommandParser_ParseAttributesCollection_ShouldReturnAttribetes() {
	// Given
	QString testScript = R"(
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
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserAttributesCollection().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
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

void TestConsoleModule::testCommandParser_ParseSingleTextValue_ShouldReturnAstValue() {
	// Given
	QString testScript = R"(
	// комментарий
	// комментарий
"тестовая строка"
)";
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserValue().parse(source, error);

	// Then
	QVERIFY(result != nullptr);

	auto srcLoc = source.currentLocation();
	auto location = result->location();
	auto srcLocation = source.currentLocation();

	auto name = result->parameter("name");
	auto value = result->parameter("value");
	auto mime = result->parameter("mime-type");

	QCOMPARE(error.has_value(), false);
	QCOMPARE_NE(source.peek(), '\"');
	QCOMPARE(location.offset, 33);
	QCOMPARE(srcLoc.offset, 50);
	QCOMPARE(name.isNull(), true);
	QCOMPARE(value.isNull(), false);
	QCOMPARE(value, "тестовая строка");
	QCOMPARE(mime, "text/plane");
}

void TestConsoleModule::testCommandParser_ParseMultilineTextValue_ShouldReturnAstValue() {
	// Given
	QString testScript = R"(
	// комментарий
	// комментарий
<<text>>
	1	тестовая строка
	2 текстовая строка
<</text>> 
)";
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserValue().parse(source, error);

	// Then
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();

	auto name = result->parameter("name");
	auto value = result->parameter("value").toString();
	auto mime = result->parameter("mime-type").toString();

	QCOMPARE(error.has_value(), false);
	QCOMPARE(name.isNull(), true);
	QCOMPARE(value.isNull(), false);
	QCOMPARE(value, R"(
	1	тестовая строка
	2 текстовая строка
)");
	QCOMPARE(mime, "text/plane");
}

void TestConsoleModule::testCommandParser_ParseInclude_ShouldReturnIncludeAstWithPath() {
	// Given
	QString testScript = R"(
	// комментарий

	#include "base/utils.script" // подключить файл
)";
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserInclude().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
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
	// Given
	QString prefix = "any block ";
	QString suffif = R"(<<text>>
	1 тестовая строка
	2 текстовая строка
<</text>> 
)";
	QString testScript = QString("%1%2")
		.arg(prefix)
		.arg(suffif);

	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	source.advance(prefix.length());
	auto block = source.tryReadQuotedBlock("<<text>>", "<</text>>", true, "", error).toUtf8();

	// Then
	QCOMPARE(error.has_value(), false);
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
