#pragma once
#include "Libs/BaseWidgets/editor/layout/text_layout.h"

#include <QFont>
#include <QString>
#include <QTextLayout>
#include <QVector>

#include <functional>

/// Позиция курсора в логической строке.
struct PagedTextPosition {
	int line = 0;
	int column = 0;

	bool operator==(const PagedTextPosition& other) const {
		return line == other.line && column == other.column;
	}
	bool operator!=(const PagedTextPosition& other) const {
		return !(*this == other);
	}
};

/// Перемещение курсора по экранным строкам с учётом WordWrap.
///
/// Функция не владеет состоянием: для многокурсорного режима её можно вызвать
/// отдельно для каждой позиции и получить новую, не затрагивая остальные.
class PagedTextCursor {
public:
	static PagedTextPosition moveVertical(const PagedTextPosition& position,
		int delta, const std::function<QString(int)>& lineText,
		const QFont& font, bool wrapped, int layoutWidth, int lineCount,
		int tabWidth = TextLayout::kDefaultTabWidth,
		const std::function<QVector<QTextLayout::FormatRange>(int)>& lineFormats = {});
};
