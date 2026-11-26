#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_cursor.h"
#include "Libs/BaseWidgets/editor/layout/text_layout.h"

#include <QtGlobal>

PagedTextPosition PagedTextCursor::moveVertical(const PagedTextPosition& position,
	int delta, const std::function<QString(int)>& lineText,
	const QFont& font, bool wrapped, int layoutWidth, int lineCount,
	int tabWidth, const std::function<QVector<QTextLayout::FormatRange>(int)>& lineFormats) {
	if (delta == 0 || lineCount <= 0
		|| position.line < 0 || position.line >= lineCount) {
		return position;
	}

	const auto formatsFor = [&lineFormats](int line) {
		return lineFormats ? lineFormats(line) : QVector<QTextLayout::FormatRange>();
	};

	const QString currentText = lineText(position.line);
	const int currentColumn = qMin(position.column, currentText.size());
	TextLayout layout(currentText, font, wrapped, layoutWidth, tabWidth,
		formatsFor(position.line));
	if (layout.lineCount() <= 0) {
		return position;
	}

	int visual = 0;
	int rowY = 0;
	int rowHeight = 0;
	if (!layout.rowForColumn(currentColumn, visual, rowY, rowHeight)) {
		return position;
	}
	const qreal cursorX = layout.xForColumn(currentColumn);

	int targetLine = position.line;
	int targetVisual = visual + delta;
	if (targetVisual < 0) {
		targetLine = position.line - 1;
		if (targetLine < 0) {
			return position;
		}
	}
	else if (targetVisual >= layout.lineCount()) {
		targetLine = position.line + 1;
		if (targetLine >= lineCount) {
			return position;
		}
	}

	TextLayout targetLayout(lineText(targetLine), font, wrapped, layoutWidth,
		tabWidth, formatsFor(targetLine));
	if (targetLayout.lineCount() <= 0) {
		return position;
	}

	if (targetLine != position.line) {
		targetVisual = delta < 0 ? targetLayout.lineCount() - 1 : 0;
	}

	targetVisual = qBound(0, targetVisual, targetLayout.lineCount() - 1);
	const int targetColumn = qBound(0,
		targetLayout.columnAt(targetVisual, cursorX),
		lineText(targetLine).size());
	return { targetLine, targetColumn };
}
