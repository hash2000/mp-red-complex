#include "Content/ConsoleModule/processors/parser/commands/parser_object.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_object.h"

ParserObject::~ParserObject() = default;


QString ParserObject::name() const {
	return "ParserObject";
}

IAstPtr ParserObject::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto startLoc = cursor.currentLocation();
	auto node = std::make_shared<AstObject>();
	node->setLocation(startLoc);
	backup.commit();
	return node;
}


