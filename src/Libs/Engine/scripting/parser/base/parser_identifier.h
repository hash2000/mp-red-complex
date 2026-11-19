#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

class ParserIdentifier : public IParser {
public:
	~ParserIdentifier() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
