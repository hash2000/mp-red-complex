#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"

class ParserCommand: public IParser {
public:
	~ParserCommand() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
