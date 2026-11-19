#pragma once
#include "Libs/Engine/scripting/parser/i_ast.h"
#include "Libs/Engine/scripting/parser/source_cursor.h"

#include <QStringList>

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

	// Отдаёт набор статических токенов
	virtual QStringList tokens() const {
		return QStringList();
	}
};
