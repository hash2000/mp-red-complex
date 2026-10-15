#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"

#include <memory>

class ParserValue: public IParser {
public:
	ParserValue();
	~ParserValue() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
