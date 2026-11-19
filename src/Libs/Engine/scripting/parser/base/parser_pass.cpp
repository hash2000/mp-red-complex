#include "Libs/Engine/scripting/parser/base/parser_pass.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"

ParserPass::~ParserPass() = default;


QString ParserPass::name() const {
	return "ParserPass";
}

IAstPtr ParserPass::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
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


