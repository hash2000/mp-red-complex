#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

class ParserProcessor: public IParser {
public:
	~ParserProcessor() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
