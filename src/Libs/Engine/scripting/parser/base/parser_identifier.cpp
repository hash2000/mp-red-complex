#include "Libs/Engine/scripting/parser/base/parser_identifier.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"
#include <QRegularExpression>

ParserIdentifier::~ParserIdentifier() = default;


QString ParserIdentifier::name() const {
	return "ParserIdentifier";
}

IAstPtr ParserIdentifier::parse(SourceCursor& cursor, std::optional<ParseError>& error) {
	SourceCursorTransaction backup(cursor);
	if (!cursor.skipWhitespaceAndComments(error)) {
		return nullptr;
	}

	SourceLocation startLoc = cursor.currentLocation();

	// Регулярное выражение для идентификатора:
	// Начинается с буквы или _, далее буквы, цифры или _
	// Или просто "*" (для SQL)
	static const QRegularExpression identifierRegex(
		R"(^[[:alpha:]_][[:alnum:]_]*|^\*)"
	);

	QStringView idView = cursor.readRegex(identifierRegex);
	if (idView.isEmpty()) {
		return nullptr;
	}

	auto name = idView.toString();
	if (name.isEmpty()) {
		error = ParseError{ startLoc, "Identifier need name." };
		return nullptr;
	}

	auto node = std::make_shared<AstBase>();
	node->setLocation(startLoc);
	node->setParameter("name", name);
	backup.commit();
	return node;
}


