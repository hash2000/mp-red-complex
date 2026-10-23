#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"

class ParserAttribute: public IParser {
public:
	~ParserAttribute() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
