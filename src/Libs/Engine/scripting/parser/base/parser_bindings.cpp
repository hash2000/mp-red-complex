#include "Libs/Engine/scripting/parser/base/parser_bindings.h"
#include "Libs/Engine/scripting/parser/base/parser_identifier.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"

class ParserBindings::Private {
public:
	Private(ParserBindings* parent) : q(parent) {}
	ParserBindings* q;

	bool tryBeginExpression(SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryEndExpression(SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryReadExpression(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error);
};


ParserBindings::ParserBindings()
	: d(std::make_unique<Private>(this)) {

}

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
	if (!d->tryBeginExpression(cursor, error)) {
		return nullptr;
	}

	auto node = std::make_shared<AstBase>();
	node->setLocation(startLoc);
	if (!d->tryReadExpression(node, cursor, error)) {
		return nullptr;
	}

	if (!d->tryEndExpression(cursor, error)) {
		return nullptr;
	}
	
	backup.commit();
	return node;
}

bool ParserBindings::Private::tryReadExpression(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	if (!cursor.skipWhitespaceAndComments(error)) {
		return false;
	}

	if (cursor.peek() != '.') {
		error = ParseError{ cursor.currentLocation(), "Expected .identifier" };
		return false;
	}
	cursor.advance();

	auto identifier = ParserIdentifier().parse(cursor, error);
	if (error) {
		return false;
	}

	if (!identifier) {
		error = ParseError{ cursor.currentLocation(), "Expected identifier name is not valid" };
		return false;
	}

	node->setParameter("bind", identifier->parameter("name"));
	return true;
}

bool ParserBindings::Private::tryBeginExpression(SourceCursor& cursor, std::optional<ParseError>& error) {
	if (!cursor.skipWhitespaceAndComments(error)) {
		return false;
	}

	if (!cursor.token("{{")) {
		return false;
	}
	return true;
}

bool ParserBindings::Private::tryEndExpression(SourceCursor& cursor, std::optional<ParseError>& error) {
	if (!cursor.skipWhitespaceAndComments(error)) {
		return false;
	}

	if (!cursor.token("}}")) {
		error = ParseError{ cursor.currentLocation(), "Expected end of expression `}}`" };
		return false;
	}

	return true;
}
