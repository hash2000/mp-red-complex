#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"
#include <memory>

class ParserBindings: public IParser {
public:
	ParserBindings();
	~ParserBindings() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
