#include "test_command.h"
#include "Libs/Engine/scripting/parser/base/parser_include.h"
#include "Libs/Engine/scripting/parser/base/parser_value.h"
#include "Libs/Engine/scripting/parser/base/parser_variable.h"
#include "Libs/Engine/scripting/parser/base/parser_attributes_collection.h"
#include "Libs/Engine/scripting/parser/base/parser_identifiers_space.h"

void TestEngine::testParser_ParseNamespace_ShouldReturnNamespacePath() {
	// Given
	QString testScript = R"(
	system
	.	text
	.  json
	. converters
)";

	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserIdentifiersSpace().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);
	QCOMPARE(result->parameter("ns-path").toString(), ".system.text.json.converters");

	int index = 0;
	std::vector<QString> map = {
		"system", "text", "json", "converters"
	};
	for (const auto attr : result->children("namespace")) {
		auto name = attr->parameter("name").toString();

		QCOMPARE_LT(index, map.size());
		QCOMPARE(name, map[index]);
		index++;
	}

	QCOMPARE(index, map.size());
}

void TestEngine::testParser_ParseNumberValue_ShouldReturnPositiveOrNegativeNumber_data() {
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

void TestEngine::testParser_ParseNumberValue_ShouldReturnPositiveOrNegativeNumber() {
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
	QCOMPARE(result->parameter("type").toString(), "number");
}


void TestEngine::testParser_ParseBoolValue_ShouldReturnTrueFalseValue_data() {
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

void TestEngine::testParser_ParseBoolValue_ShouldReturnTrueFalseValue() {
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
	QCOMPARE(result->parameter("type").toString(), "bool");
}

void TestEngine::testParser_ParseVariable_ShouldCheckRValue_data() {
	QTest::addColumn<QString>("testScript");
	QTest::addColumn<bool>("forseRValue");
	QTest::addColumn<bool>("errorState");

	QTest::newRow("var key") << R"(
	var key
)" << false << true;

	QTest::newRow("var key: 123") << R"(
	var key: 123
)" << false << false;

	QTest::newRow("vec[rgb, Window, Active, darker=100] anyStyle: (120, 125, 125)") << R"(
	vec[rgb, Window, Active, darker=100] anyStyle: (120, 125, 125)
)" << true << false;

	QTest::newRow("vec[rgb, Window, Active, darker=100] anyStyle") << R"(
	vec[rgb, Window, Active, darker=100] anyStyle
)" << true << true;
}

void TestEngine::testParser_ParseVariable_ShouldCheckRValue() {
// Given
	QFETCH(QString, testScript);
	QFETCH(bool, forseRValue);
	QFETCH(bool, errorState);


	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserVariable(forseRValue).parse(source, error);

	// Then
	QCOMPARE(error.has_value(), errorState);
}

void TestEngine::testParser_ParseVariableWithBinding_ShouldReturnVariableDeclarationWithBindName() {
	// Given
	QString testScript = R"(
	// тип данных (string, number, bool)
	string key:{{.anotherValue}};
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
	QCOMPARE(result->parameter("type").toString(), "string");
}

void TestEngine::testParser_ParseTypedVariable_ShouldReturnVariableDeclarationWithoutValue() {
	// Given
	QString testScript = R"(
	// тип данных (string, number, bool)
	string[secret, mime="text/plane", rare] key;
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
	QCOMPARE(result->parameter("type").toString(), "string");
}

void TestEngine::testParser_ParseTypedVariableWithValue_ShouldRequridTypeMustCorrecponded_data() {
	QTest::addColumn<QString>("testScript");
	QTest::addColumn<QString>("expectedType");
	QTest::addColumn<QString>("expectedMimeType");

	QTest::addRow("string & text/jason") << R"(
	// тип данных
	string[any_attribute="param"] key: <<json>>{"key1":"val1","key2":"val2"}<</json>>;
)" << "string" << "application/json";

	QTest::addRow("string & application/xml") << R"(
	// тип данных
	string[any_attribute="param"] key: <<xml>>
<root>
		<head></head>
		<body></body>
</root>
<</xml>>;
)" << "string" << "text/xml";

	QTest::addRow("string & text/x-ini") << R"(
	// тип данных
	string[any_attribute="param"] key: <<ini>>
[Settings]
test1:val1
test2:val2
<</ini>>;
)" << "string" << "text/x-ini";

	QTest::addRow("string & multiline text/plane") << R"(
	// тип данных
	string[any_attribute="param"] key: <<text>>
		any
		multiline
		text
<</text>>;
)" << "string" << "text/plane";

	QTest::addRow("string & single line text/plane") << R"(
	// тип данных
	string[any_attribute="param"] key: "single line text";
)" << "string" << "text/plane";

	QTest::addRow("number") << R"(
	// тип данных
	number[any_attribute="param"] key: 100.0;
)" << "number" << "";

	QTest::addRow("AnyObject") << R"(
	// тип данных
	AnyObject[any_attribute="param"] key: 100.0;
)" << "AnyObject" << "";
}

void TestEngine::testParser_ParseTypedVariableWithValue_ShouldRequridTypeMustCorrecponded() {
	// Given
	QFETCH(QString, testScript);
	QFETCH(QString, expectedType);
	QFETCH(QString, expectedMimeType);
	SourceCursor source(testScript);
	std::optional<ParseError> error;

	// When
	auto result = ParserVariable().parse(source, error);

	// Then
	QCOMPARE(error.has_value(), false);
	QVERIFY(result != nullptr);

	auto location = result->location();
	auto srcLocation = source.currentLocation();
	auto mimeType = result->parameter("mime-type").toString();

	QCOMPARE(result->parameter("name").toString(), "key");
	QCOMPARE(result->parameter("value").isNull(), false);
	QCOMPARE(result->parameter("type").toString(), expectedType);
	QCOMPARE(mimeType, expectedMimeType);
}


void TestEngine::testParser_ParseTypedVariableWithValue_ShouldCompareLTypeAndRType() {
	// Given
	QString testScript = R"(
	// тип данных (string, number, bool)
	string[secret, mime="text/jason", rare] key;
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
	QCOMPARE(result->parameter("type").toString(), "string");
}

void TestEngine::testParser_ParseRValueTypedVariable_ShouldReturnVariableDeclarationWithValue() {
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
	QCOMPARE(result->parameter("type").toString(), "var");
	
}

void TestEngine::testParser_ParseAttributesCollection_ShouldReturnAttribetes() {
	// Given
	QString testScript = R"(
	[
		attr1=1,
		attr2=<<text>>
	parameter value
<</text>>,
		attr3=true,
		attr4=false
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

	QCOMPARE(result->children("any-group").size(), 0);
	QCOMPARE(result->children("attributes").size(), 4);

	std::set<QString> found;
	for (const auto attr : result->children("attributes")) {
		auto name = attr->parameter("name").toString();
		auto inserResult = found.emplace(name);
		QCOMPARE(inserResult.second, true);

		if (name == "attr1") {
			QCOMPARE(attr->parameter("type").toString(), "number");
			QCOMPARE(attr->parameter("mime-type").isNull(), true);
			QCOMPARE(attr->parameter("value").toInt(), 1);
		}
		else if (name == "attr2") {
			QCOMPARE(attr->parameter("type").toString(), "string");
			QCOMPARE(attr->parameter("mime-type").toString(), "text/plane");
			QCOMPARE(attr->parameter("value").toString(), R"(
	parameter value
)");
		}
		else if (name == "attr3") {
			QCOMPARE(attr->parameter("type").toString(), "bool");
			QCOMPARE(attr->parameter("mime-type").isNull(), true);
			QCOMPARE(attr->parameter("value").toBool(), true);
		}
		else if (name == "attr4") {
			QCOMPARE(attr->parameter("type").toString(), "bool");
			QCOMPARE(attr->parameter("mime-type").isNull(), true);
			QCOMPARE(attr->parameter("value").toBool(), false);
		}
	}

	QCOMPARE(found.size(), 4);
}

void TestEngine::testParser_ParseSingleTextValue_ShouldReturnAstValue() {
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

void TestEngine::testParser_ParseMultilineTextValue_ShouldReturnAstValue() {
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

void TestEngine::testParser_ParseInclude_ShouldReturnIncludeAstWithPath() {
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

void TestEngine::testParser_SourceCursorReadUntil_ShouldReturnTextBetweenQuotesMarkers() {
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
