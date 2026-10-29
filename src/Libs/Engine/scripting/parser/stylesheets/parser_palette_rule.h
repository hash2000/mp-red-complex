#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

#include <memory>

class ParserPaletteRule : public IParser {
public:
	~ParserPaletteRule() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
};
