#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

class ParserAttributesCollection: public IParser {
public:
	~ParserAttributesCollection() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
