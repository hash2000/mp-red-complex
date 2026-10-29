#include "Libs/Engine/scripting/parser/base/parser_attribute.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"
#include "Libs/Engine/scripting/parser/base/ast_variable_defs.h"
#include "Libs/Engine/scripting/parser/base/parser_value.h"
#include "Libs/Engine/scripting/parser/base/ast_variable_defs.h"
#include "Libs/Engine/scripting/parser/base/parser_identifier.h"

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

	auto node = std::make_shared<AstBase>();
	node->setLocation(startLoc);
	node->setParameter("name", ident->parameter("name"));

	if (cursor.peek() == '(') {
		cursor.advance(); // Съедаем (

		ParserValue valueParser;

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


