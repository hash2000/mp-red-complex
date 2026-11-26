#pragma once
#include "Libs/BaseWidgets/extensions/text_edit_extensions.h"

#include <QString>
#include <memory>

class CommandContext;

/// Глобальные параметры текстовых документов (`documents.text.*`).
///
/// Значения — переменные глобального контекста, которые документ видит через
/// свой контекст. Снимок читается при создании документа (когда к нему
/// привязывается контекст) и применяется как значения по умолчанию; дальше
/// документ может менять их у себя, не затрагивая глобальные.
struct TextDocumentOptions {
	/// Семейство шрифта; пустая строка — системный моноширинный.
	QString fontFamily;
	/// Размер шрифта в пунктах; 0 — системный.
	int fontSize = 0;
	/// Перенос длинных строк.
	bool wordWrap = false;
	/// Кодировка чтения и записи (имя кодека QTextCodec).
	QString encoding;
	/// Предел буфера виртуального редактора в мегабайтах; 0 — без предела.
	int bufferLimitMb = 0;
	/// Ширина отступа и табуляции в символах (Prettier tabWidth).
	int tabSpaces = 2;
	/// Что вставлять по Tab (Prettier useTabs).
	Extensions::TextEdit::TabMode tabEdit = Extensions::TextEdit::TabMode::Spaces;
	/// Как записывать ведущие табы при сохранении.
	Extensions::TextEdit::TabMode tabSave = Extensions::TextEdit::TabMode::Spaces;
	/// Идентификатор стратегии перемещения по токенам.
	QString navigation = QStringLiteral("words");

	static TextDocumentOptions fromContext(
		const std::shared_ptr<CommandContext>& context);
};
