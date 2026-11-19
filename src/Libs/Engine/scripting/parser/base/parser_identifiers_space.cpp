#include "Libs/Engine/scripting/parser/base/parser_identifiers_space.h"
#include "Libs/Engine/scripting/parser/base/parser_identifier.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"

ParserIdentifiersSpace::~ParserIdentifiersSpace() = default;


QString ParserIdentifiersSpace::name() const {
	return "ParserIdentifiersSpace";
}

IAstPtr ParserIdentifiersSpace::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto node = std::make_shared<AstBase>();
	node->setLocation(cursor.currentLocation());

	QString full;
	ParserIdentifier identParser;
	while (!cursor.isEnd()) {
		auto name = identParser.parse(cursor, error);
		if (!name || error) {
			return nullptr;
		}

		node->setChild("namespace", name);
		full += QString(".%1")
			.arg(name->parameter("name").toString());

		if (!cursor.skipWhitespaceAndComments(error)) {
			return nullptr;
		}

		if (cursor.peek() != '.') {
			break;
		}
		cursor.advance();
	}

	node->setParameter("ns-path", full);

	backup.commit();
	return node;
}


