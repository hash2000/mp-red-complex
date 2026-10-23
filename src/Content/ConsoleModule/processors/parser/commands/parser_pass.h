#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"

class ParserPass: public IParser {
public:
	~ParserPass() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
