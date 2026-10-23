#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"

class ParserVariable: public IParser {
public:
	ParserVariable();
	~ParserVariable() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
	QStringList tokens() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
