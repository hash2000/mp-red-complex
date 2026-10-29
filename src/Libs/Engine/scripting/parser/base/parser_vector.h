#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

class ParserVector: public IParser {
public:
	~ParserVector() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
