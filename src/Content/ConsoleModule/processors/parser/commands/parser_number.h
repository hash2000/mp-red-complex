#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_variable_defs.h"

class ParserNumber : public IParser {
public:
	~ParserNumber() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
