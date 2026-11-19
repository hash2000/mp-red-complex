#include "Libs/Engine/scripting/parser/base/parser_attribute.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"
#include "Libs/Engine/scripting/parser/base/parser_value.h"
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

	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	if (cursor.peek() == '=') {
		cursor.advance();

		auto value = ParserValue().parse(cursor, error);
		if (!value || error) {
			return nullptr;
		}

		node->setParameter("value", value->parameter("value"));
		node->setParameter("type", value->parameter("type"));
		node->setParameter("mime-type", value->parameter("mime-type"));
	}

	backup.commit();
	return node;
}


