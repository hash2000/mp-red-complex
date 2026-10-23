#include "Content/ConsoleModule/processors/parser/commands/parser_include.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_include.h"
#include "Content/ConsoleModule/processors/parser/source_cursor.h"

ParserInclude::~ParserInclude() = default;


QString ParserInclude::name() const {
	return "ParserInclude";
}

QStringList ParserInclude::tokens() const {
	return {
		"#include"
	};
}

IAstPtr ParserInclude::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto startLoc = cursor.currentLocation();

	if (!cursor.token("#include")) {
		return nullptr;
	}

	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto path = cursor.tryReadQuotedBlock("\"", false, "include path is empty", error);
	if (path.isEmpty()) {
		return nullptr;
	}

	auto node = std::make_shared<AstInclude>();
	node->setLocation(startLoc);
	node->setParameter("path", path.toString());
	backup.commit();
	return node;
}


