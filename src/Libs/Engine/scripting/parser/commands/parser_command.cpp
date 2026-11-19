#include "Libs/Engine/scripting/parser/commands/parser_command.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"
#include "Libs/Engine/scripting/parser/base/parser_value.h"

#include <QRegularExpression>

namespace {

/// Имя команды и параметра: буква или `_`, далее буквы, цифры, `_`, `.` и `-`.
/// Точка нужна для namespace-команд, дефис — для имён вида `fetch-api`.
const QRegularExpression& commandNameRegex() {
	static const QRegularExpression regex(R"(^[[:alpha:]_][[:alnum:]_.-]*)");
	return regex;
}

/// Символ значения без кавычек: значение читается до пробела или запятой.
bool isValueChar(QChar ch) {
	return !ch.isSpace() && ch != u',';
}

/// Разделитель параметров или конец строки.
bool isParameterBoundary(QChar ch) {
	return ch.isSpace() || ch == u',';
}

} // namespace

class ParserCommand::Private {
public:
	IAstPtr parseParameter(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr parseValue(SourceCursor& cursor, std::optional<ParseError>& error);
	IAstPtr parseBareValue(SourceCursor& cursor, std::optional<ParseError>& error);
};

ParserCommand::ParserCommand()
	: d(std::make_unique<Private>()) {
}

ParserCommand::~ParserCommand() = default;

QString ParserCommand::name() const {
	return "ParserCommand";
}

IAstPtr ParserCommand::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	if (cursor.isEnd()) {
		return nullptr;
	}

	const auto startLocation = cursor.currentLocation();
	const auto commandName = cursor.readRegex(commandNameRegex());
	if (commandName.isEmpty()) {
		error = ParseError{ cursor.currentLocation(), "Command name expected" };
		return nullptr;
	}

	auto node = std::make_shared<AstBase>();
	node->setLocation(startLocation);
	node->setParameter("name", commandName.toString());

	while (true) {
		if (!cursor.skipWhitespaceAndComments(error)) {
			return nullptr;
		}

		if (cursor.isEnd()) {
			break;
		}

		auto parameter = d->parseParameter(cursor, error);
		if (!parameter) {
			return nullptr;
		}

		node->setChild("parameter", parameter);

		if (!cursor.skipWhitespaceAndComments(error)) {
			return nullptr;
		}

		if (cursor.isEnd()) {
			break;
		}

		// Параметры разделяются запятой.
		if (cursor.peek() != u',') {
			error = ParseError{ cursor.currentLocation(), "Expected ',' between parameters" };
			return nullptr;
		}

		cursor.advance();
	}

	backup.commit();
	return node;
}

IAstPtr ParserCommand::Private::parseParameter(SourceCursor& cursor, std::optional<ParseError>& error) {
	const auto startLocation = cursor.currentLocation();
	const auto parameterName = cursor.readRegex(commandNameRegex());
	if (parameterName.isEmpty()) {
		error = ParseError{ startLocation, "Parameter name expected" };
		return nullptr;
	}

	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	if (!cursor.tryToken("=", "Expected '=' after parameter name", error)) {
		return nullptr;
	}

	auto value = parseValue(cursor, error);
	if (!value) {
		return nullptr;
	}

	auto node = std::make_shared<AstBase>();
	node->setLocation(startLocation);
	node->setParameter("name", parameterName.toString());
	node->setParameter("value", value->parameter("value"));

	// Числа, bool и векторы не задают mime-тип: для консоли считаем их текстом.
	auto mimeType = value->parameter("mime-type").toString();
	if (mimeType.isEmpty()) {
		mimeType = "text/plane";
	}

	node->setParameter("mime-type", mimeType);
	return node;
}

IAstPtr ParserCommand::Private::parseValue(SourceCursor& cursor, std::optional<ParseError>& error) {
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	// Значение без текста: `param=` в конце строки или перед запятой.
	if (cursor.isEnd() || isParameterBoundary(cursor.peek())) {
		auto node = std::make_shared<AstBase>();
		node->setLocation(cursor.currentLocation());
		node->setParameter("value", QVariant());
		node->setParameter("mime-type", "text/plane");
		return node;
	}

	// Блоки значений задаются только в формате DSL: `<<json>>...<</json>>`.
	if (cursor.startsWith(QStringLiteral("<")) && !cursor.startsWith(QStringLiteral("<<"))) {
		error = ParseError{ cursor.currentLocation(),
			"Block values use the <<json>>...<</json>> syntax" };
		return nullptr;
	}

	// Значения языка DSL: кавычки, числа, bool, `<<json>>`-блоки.
	{
		SourceCursorTransaction valueTransaction(cursor);
		auto value = ParserValue().parse(cursor, error);
		if (error.has_value()) {
			return nullptr;
		}

		// Значение DSL не должно сливаться с остальным текстом (`123abc`).
		if (value && (cursor.isEnd() || isParameterBoundary(cursor.peek()))) {
			valueTransaction.commit();
			return value;
		}
	}

	return parseBareValue(cursor, error);
}

IAstPtr ParserCommand::Private::parseBareValue(SourceCursor& cursor, std::optional<ParseError>& error) {
	const auto startLocation = cursor.currentLocation();
	const auto value = cursor.readWhile(isValueChar);
	if (value.isEmpty()) {
		error = ParseError{ startLocation, "Parameter value expected" };
		return nullptr;
	}

	auto node = std::make_shared<AstBase>();
	node->setLocation(startLocation);
	node->setParameter("value", value.toString());
	node->setParameter("mime-type", "text/plane");
	return node;
}
