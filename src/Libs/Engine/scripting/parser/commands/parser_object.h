#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

class ParserObject: public IParser {
public:
	~ParserObject() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
