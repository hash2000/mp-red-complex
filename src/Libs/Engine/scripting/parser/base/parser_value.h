#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

#include <QString>

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
