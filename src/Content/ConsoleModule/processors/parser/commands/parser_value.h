#pragma once
#include "Content/ConsoleModule/processors/parser/i_parser.h"
#include "Content/ConsoleModule/processors/parser/commands/ast_variable_defs.h"

#include <QString>

#include <memory>


class ParserValue: public IParser {
public:
	ParserValue(AstDataType expectedType, const QString& expectedMimeType = QString());
	~ParserValue() override;
	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
