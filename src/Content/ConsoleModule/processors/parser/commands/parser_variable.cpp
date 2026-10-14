#include "Content/ConsoleModule/processors/parser/commands/parser_variable.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_variable.h"


class ParserVariable::Private {
public:
	Private(ParserVariable* parent) : q(parent) { }
	ParserVariable* q;

	bool tryPopulateType(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryPopulateAttributes(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error);
};

ParserVariable::ParserVariable()
: d(std::make_unique<Private>(this)) {
}

ParserVariable::~ParserVariable() = default;


bool ParserVariable::Private::tryPopulateType(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	node->needRVal = false;
	if (cursor.token("var")) {
		node->needRVal = true;
	}
	else if (cursor.token("int")) {
		node->type = "int";
	}
	else if (cursor.token("double")) {
		node->type = "double";
	}
	else if (cursor.token("bool")) {
		node->type = "bool";
	}
	else if (cursor.token("string")) {
		node->type = "string";
	}
	else {
		return false;
	}

	return true;
}

bool ParserVariable::Private::tryPopulateAttributes(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error) {


	return true;
}

QString ParserVariable::name() const {
	return "ParserVariable";
}

QStringList ParserVariable::tokens() const {
	return { "var", "int", "double", "bool", "string" };
}

IAstPtr ParserVariable::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	bool needRVal = false;
	auto startLoc = cursor.currentLocation();
	auto node = std::make_shared<AstVariable>();

	if (!d->tryPopulateType(node, cursor, error)) {
		return nullptr;
	}

	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	d->tryPopulateAttributes(node, cursor, error);

	node->setLocation(startLoc);
	backup.commit();
	return node;
}


