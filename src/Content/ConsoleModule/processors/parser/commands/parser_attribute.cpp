#include "Content/ConsoleModule/processors/parser/commands/parser_attribute.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_attribute.h"

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





	auto node = std::make_shared<AstAttribute>();
	node->setLocation(startLoc);
	backup.commit();
	return node;
}


