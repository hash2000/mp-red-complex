#include "Content/ConsoleModule/processors/parser/commands/parser_attributes_collection.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_attribute.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_attributes_collection.h"

ParserAttributesCollection::~ParserAttributesCollection() = default;


QString ParserAttributesCollection::name() const {
	return "ParserAttributesCollection";
}

IAstPtr ParserAttributesCollection::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto startLoc = cursor.currentLocation();
	auto ch = cursor.peek();
	if (ch != '[') {
		return nullptr;
	}
	cursor.advance();

	auto node = std::make_shared<AstAttributesCollection>();
	node->setLocation(startLoc);

	while (1) {
		ch = cursor.peek();
		if (ch == ']') {
			// end of collection
			cursor.advance();
			break;
		}

		if (cursor.isEnd()) {
			error = ParseError{ startLoc, "Unexpected end of attributes collection." };
			return nullptr;
		}

		auto attr = ParserAttribute().parse(cursor, error);
		if (!attr) {
			// error reading attribute
			return nullptr;
		}

		node->setChild("attributes", attr);

		if (!cursor.skipWhitespaceAndComments(error)) {
			return nullptr;
		}

		if (cursor.peek() == ',') {
			cursor.advance(); // Съедаем ,
		}
		else if (cursor.peek() != ']') {
			error = ParseError{ cursor.currentLocation(), "Expected ',' or ']'" };
			return nullptr;
		}
	}

	backup.commit();
	return node;
}


