#include "Content/ConsoleModule/processors/parser/commands/parser_variable.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_variable.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_attributes_collection.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_identifier.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_value.h"
#include "Content/ConsoleModule/processors/parser/commands/parser_bindings.h"

class ParserVariable::Private {
public:
	Private(ParserVariable* parent) : q(parent) { }
	ParserVariable* q;

	bool tryPopulateType(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryPopulateAttributes(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryPopulateValues(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error);
	bool tryPopulateVariableName(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error);
};

ParserVariable::ParserVariable()
: d(std::make_unique<Private>(this)) {
}

ParserVariable::~ParserVariable() = default;


bool ParserVariable::Private::tryPopulateType(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	bool needRVal = false;
	AstDataType type = AstDataType::Undefined;
	if (cursor.token("var")) {
		needRVal = true;
		type = AstDataType::Any;
	}
	else if (cursor.token("number")) {
		type = AstDataType::Number;
	}
	else if (cursor.token("bool")) {
		type = AstDataType::Bool;
	}
	else if (cursor.token("string")) {
		type = AstDataType::String;
	}
	else {
		return false;
	}

	node->setParameter("need-rvalue", needRVal);
	node->setParameter("type", QVariant::fromValue(static_cast<size_t>(type)));

	return true;
}

bool ParserVariable::Private::tryPopulateVariableName(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	auto loc = cursor.currentLocation();
	auto variable = ParserIdentifier().parse(cursor, error);
	if (!variable) {
		error = ParseError{ loc, "Expected variable name." };
		return false;
	}

	node->setParameter("name", variable->parameter("name"));
	return true;
}

bool ParserVariable::Private::tryPopulateAttributes(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	auto attribs = ParserAttributesCollection().parse(cursor, error);
	if (!attribs) {
		return !error.has_value();
	}

	node->setChildren("attributes", attribs->children("attributes"));

	return true;
}

bool ParserVariable::Private::tryPopulateValues(std::shared_ptr<AstVariable> node, SourceCursor& cursor, std::optional<ParseError>& error) {
	auto requiredMimeType = node->parameter("mime-type").toString();
	auto requiredType = static_cast<AstDataType>(node->parameter("type").value<size_t>());
	auto val = ParserValue(requiredType, requiredMimeType).parse(cursor, error);
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
	return { "var", "int", "double", "bool", "string" };
}

IAstPtr ParserVariable::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	auto node = std::make_shared<AstVariable>();
	node->setLocation(cursor.currentLocation());

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


