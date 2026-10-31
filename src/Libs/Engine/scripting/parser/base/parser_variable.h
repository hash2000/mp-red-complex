#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

class ParserVariable: public IParser {
public:
	ParserVariable(bool forseNeedValue = false);
	~ParserVariable() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;
	QStringList tokens() const override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
