#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"

class ParserBindings: public IParser {
public:
	~ParserBindings() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
