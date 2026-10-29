#include "Libs/Engine/scripting/parser/base/parser_vector.h"
#include "Libs/Engine/scripting/parser/base/parser_number.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"

ParserVector::~ParserVector() = default;


QString ParserVector::name() const {
	return "ParserVector";
}

IAstPtr ParserVector::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto startLoc = cursor.currentLocation();

	if (cursor.peek() != '(') {
		return nullptr;
	}
	cursor.advance();

	auto x = ParserNumber().parse(cursor, error);
	if (x == nullptr || error.has_value()) {
		return nullptr;
	}

	if (!cursor.skipWhitespaceAndComments(error) || cursor.peek() != ',') {
		return nullptr;
	}
	cursor.advance();

	auto y = ParserNumber().parse(cursor, error);
	if (y == nullptr || error.has_value()) {
		return nullptr;
	}

	if (!cursor.skipWhitespaceAndComments(error) || cursor.peek() != ',') {
		return nullptr;
	}
	cursor.advance();

	auto z = ParserNumber().parse(cursor, error);
	if (z == nullptr || error.has_value()) {
		return nullptr;
	}

	if (!cursor.skipWhitespaceAndComments(error) || cursor.peek() != ')') {
		return nullptr;
	}
	cursor.advance();

	auto node = std::make_shared<AstBase>();
	node->setParameter("x", x->parameter("value"));
	node->setParameter("y", y->parameter("value"));
	node->setParameter("z", z->parameter("value"));
	node->setParameter("type", "vec");
	node->setLocation(startLoc);
	backup.commit();
	return node;
}


