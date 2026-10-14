#include "Content/ConsoleModule/processors/parser/commands/parser_command.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_context.h"
#include "Content/ConsoleModule/processors/parser/source_cursor.h"


ParserCommand::~ParserCommand() = default;


QString ParserCommand::name() const {
	return "CommandParser";
}

IAstPtr ParserCommand::parse(SourceCursor& cursor, std::optional<ParseError>& error) {

	return nullptr;
}


