#include "Libs/Engine/scripting/parser/base/parser_variable.h"
#include "Libs/Engine/scripting/parser/base/parser_value.h"
#include "Libs/Engine/scripting/parser/base/parser_identifier.h"
#include "Libs/Engine/scripting/parser/base/parser_attributes_collection.h"
#include "Libs/Engine/scripting/parser/base/parser_bindings.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"
class ParserVariable::Private {
public:
	Private(ParserVariable* parent) : q(parent) { }
	ParserVariable* q;
	bool forseNeedValue;

	bool tryPopulateType(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryPopulateAttributes(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryPopulateValues(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryPopulateVariableName(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error);
};

ParserVariable::ParserVariable(bool forseNeedValue)
: d(std::make_unique<Private>(this)) {
	d->forseNeedValue = forseNeedValue;
}

ParserVariable::~ParserVariable() = default;


bool ParserVariable::Private::tryPopulateType(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	bool needRVal = false;
	const auto type = ParserIdentifier().parse(cursor, error);
	if (error || !type) {
		return false;
	}

	const auto typeName = type->parameter("name").toString();
	
	if (typeName == "var") {
		needRVal = true;
	}

	node->setParameter("need-rvalue", needRVal, false);
	node->setParameter("type", typeName);

	return true;
}

bool ParserVariable::Private::tryPopulateVariableName(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	auto loc = cursor.currentLocation();
	auto variable = ParserIdentifier().parse(cursor, error);
	if (!variable) {
		error = ParseError{ loc, "Expected variable name." };
		return false;
	}

	node->setParameter("name", variable->parameter("name"));
	return true;
}

bool ParserVariable::Private::tryPopulateAttributes(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	auto attribs = ParserAttributesCollection().parse(cursor, error);
	if (!attribs) {
		return !error.has_value();
	}

	node->setChildren("attributes", attribs->children("attributes"));

	return true;
}

bool ParserVariable::Private::tryPopulateValues(std::shared_ptr<AstBase> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	auto val = ParserValue().parse(cursor, error);
	if (error.has_value()) {
		return false;
	}

	if (!val) {
		auto bind = ParserBindings().parse(cursor, error);
		if (!bind) {
			return false;
		}

		node->setParameter("bind", bind->parameter("bind"));
		return true;
	}

	node->setParameter("value", val->parameter("value"));
	node->setParameter("mime-type", val->parameter("mime-type"));
	return true;
}

QString ParserVariable::name() const {
	return "ParserVariable";
}

QStringList ParserVariable::tokens() const {
	return { "var", "number", "bool", "string", "vec" };
}

IAstPtr ParserVariable::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto node = std::make_shared<AstBase>();
	node->setLocation(cursor.currentLocation());

	if (d->forseNeedValue) {
		node->setParameter("need-rvalue", true, false);
	}

	if (!d->tryPopulateType(node, cursor, error)) {
		return nullptr;
	}

	if (!d->tryPopulateAttributes(node, cursor, error)) {
		return nullptr;
	}

	if (!d->tryPopulateVariableName(node, cursor, error)) {
		return nullptr;
	}

	auto loc = cursor.currentLocation();
	auto needRValue = node->parameter("need-rvalue").toBool();
	if (cursor.peek() == ';') {
		if (needRValue) {
			error = ParseError(loc, QString("Expected rvalue to variable %1.")
				.arg(node->parameter("name").toString()));
			return nullptr;
		}

		cursor.advance();
		backup.commit();
		return node;
	}

	if (cursor.peek() != ':') {
		if (needRValue) {
			error = ParseError(loc, QString("Expected rvalue to variable %1.")
				.arg(node->parameter("name").toString()));
			return nullptr;
		}

		error = ParseError(loc, QString("Expected semicolon %1.")
			.arg(node->parameter("name").toString()));
		return nullptr;
	}

	cursor.advance();

	if (!d->tryPopulateValues(node, cursor, error)) {
		return nullptr;
	}

	backup.commit();
	return node;
}


