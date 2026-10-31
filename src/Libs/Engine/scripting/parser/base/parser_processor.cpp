#include "Libs/Engine/scripting/parser/base/parser_processor.h"
#include "Libs/Engine/scripting/parser/base/parser_identifiers_space.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"

ParserProcessor::~ParserProcessor() = default;


QString ParserProcessor::name() const {
	return "ParserProcessor";
}

IAstPtr ParserProcessor::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	if (!cursor.token("processor")) {
		return nullptr;
	}

	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto ns = ParserIdentifiersSpace().parse(cursor, error);
	if (!ns || error) {
		return nullptr;
	}




	auto node = std::make_shared<AstBase>();
	node->setLocation(cursor.currentLocation());
	node->setChildren("namespace", ns->children("namespace"));
	node->setParameter("ns-path", ns->parameter("ns-path"));

	backup.commit();
	return node;
}


