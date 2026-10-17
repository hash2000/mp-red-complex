#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"

class ParserObject: public IParser {
public:
	~ParserObject() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
