#include "Libs/Engine/scripting/parser/commands/parser_object.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"

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
	auto node = std::make_shared<AstBase>();
	node->setLocation(startLoc);
	backup.commit();
	return node;
}


