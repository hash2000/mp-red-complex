#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

class ParserCommand: public IParser {
public:
	~ParserCommand() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
