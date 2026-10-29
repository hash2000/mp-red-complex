#include "Libs/Engine/scripting/parser/commands/parser_command.h"
#include "Libs/Engine/scripting/parser/base/ast_base.h"
#include "Libs/Engine/scripting/parser/source_cursor.h"


ParserCommand::~ParserCommand() = default;


QString ParserCommand::name() const {
	return "CommandParser";
}

IAstPtr ParserCommand::parse(SourceCursor& cursor, std::optional<ParseError>& error) {

	return nullptr;
}


