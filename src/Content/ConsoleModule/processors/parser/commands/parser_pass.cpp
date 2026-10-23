#include "Content/ConsoleModule/processors/parser/commands/parser_pass.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_pass.h"

ParserPass::~ParserPass() = default;


QString ParserPass::name() const {
	return "ParserPass";
}

IAstPtr ParserPass::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	auto startLoc = cursor.currentLocation();
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto node = std::make_shared<AstPass>();
	node->setLocation(startLoc);
	backup.commit();
	return node;
}


