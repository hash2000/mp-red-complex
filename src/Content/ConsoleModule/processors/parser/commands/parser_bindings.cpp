#include "Content/ConsoleModule/processors/parser/commands/parser_bindings.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_bindings.h"

ParserBindings::~ParserBindings() = default;


QString ParserBindings::name() const {
	return "ParserBindings";
}

IAstPtr ParserBindings::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto startLoc = cursor.currentLocation();
	auto result = cursor.tryReadQuotedBlock("{{", "}}", false, "Read bindings", error);
	if (result.isEmpty()) {
		return nullptr;
	}

	auto node = std::make_shared<AstBindings>();
	node->setLocation(startLoc);
	node->setParameter("bind", result.toString());
	backup.commit();
	return node;
}


