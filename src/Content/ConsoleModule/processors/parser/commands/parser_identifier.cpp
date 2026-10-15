#include "Content/ConsoleModule/processors/parser/commands/parser_identifier.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_identifier.h"
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

	auto node = std::make_shared<AstIdentifier>();
	node->setLocation(startLoc);
	node->setParameter("name", idView.toUtf8());
	backup.commit();
	return node;
}


