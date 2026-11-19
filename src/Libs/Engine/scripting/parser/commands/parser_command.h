#pragma once
#include "Libs/Engine/scripting/parser/i_parser.h"

#include <memory>

/// @brief Разбирает строку вызова команды: `command name=value, name2=value2`.
///
/// Параметры читаются так же, как атрибуты в `ParserAttribute`: имя, `=`,
/// значение через `ParserValue`. Параметры разделяются запятой.
///
/// Результат разбора — узел с параметром `name` (имя команды) и детьми типа
/// `parameter`. Каждый ребёнок содержит параметры `name`, `value` и
/// `mime-type` (mime-тип значения).
class ParserCommand : public IParser {
public:
	ParserCommand();
	~ParserCommand() override;

	QString name() const override;
	IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) override;

private:
	class Private;
	std::unique_ptr<Private> d;
};
