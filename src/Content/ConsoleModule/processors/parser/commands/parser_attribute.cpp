#include "Content/ConsoleModule/processors/parser/commands/parser_attribute.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_attribute.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_variable_defs.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_identifier.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_value.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_variable_defs.h"

ParserAttribute::~ParserAttribute() = default;


QString ParserAttribute::name() const {
	return "ParserAttribute";
}

IAstPtr ParserAttribute::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	SourceLocation startLoc = cursor.currentLocation();
	auto ident = ParserIdentifier().parse(cursor, error);
	if (!ident) {
		return nullptr;
	}

	auto node = std::make_shared<AstAttribute>();
	node->setLocation(startLoc);
	node->setParameter("name", ident->parameter("name"));

	if (cursor.peek() == '(') {
		cursor.advance(); // Съедаем (

		ParserValue valueParser(AstDataType::Any);

		while (!cursor.isEnd()) {
			if (!cursor.skipWhitespaceAndComments(error)) {
				return nullptr;
			}

			// Проверяем конец списка параметров
			if (cursor.peek() == ')') {
				cursor.advance(); // Съедаем )
				break;
			}

			// Парсим значение параметра
			auto paramValue = valueParser.parse(cursor, error);
			if (!paramValue) {
				if (error.has_value()) {
					return nullptr;
				}
				error = ParseError{ cursor.currentLocation(), "Expected parameter value." };
				return nullptr;
			}

			node->setChild("parameters", paramValue);

			if (!cursor.skipWhitespaceAndComments(error)) {
				return nullptr;
			}

			// Проверяем разделитель (запятая) или конец
			if (cursor.peek() == ',') {
				cursor.advance(); // Съедаем ,
			}
			else if (cursor.peek() != ')') {
				error = ParseError{ cursor.currentLocation(), "Expected ',' or ')'" };
				return nullptr;
			}
		}
	}

	backup.commit();
	return node;
}


