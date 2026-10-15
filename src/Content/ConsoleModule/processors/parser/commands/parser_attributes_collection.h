#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"

class ParserAttributesCollection: public IParser {
public:
	~ParserAttributesCollection() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
