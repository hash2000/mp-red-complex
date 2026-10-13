#pragma once
#include "Content/ConsoleModule/processors/parser/i_ast.h"
#include "Content/ConsoleModule/processors/parser/source_cursor.h"

class IParser {
public:
	virtual ~IParser() = default;

	// Имя парсера (для отладки и логов)
	virtual QString name() const = 0;

	// Пытается распарсить узел. 
	// Возвращает IAstPtr при успехе. 
	// Возвращает nullptr, если текущий текст не подходит под этот парсер.
	// Если текст подходил, но синтаксис нарушен, заполняет `error` и возвращает nullptr.
	virtual IAstPtr parse(SourceCursor& cursor, std::optional<ParseError>& error) = 0;
};
