#pragma once
#include "Libs/BaseWidgets/editor/layout/text_layout.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_search.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_highlighter.h"

#include <QFont>
#include <QHash>
#include <QPalette>
#include <QRect>
#include <QString>
#include <QVector>

#include <functional>

class QPainter;

/// Каретка одного курсора.
struct PagedTextCaret {
	int line = 0;
	int column = 0;
};

/// Выделение одного курсора (нормализованный диапазон).
struct PagedTextSelection {
	int startLine = 0;
	int startColumn = 0;
	int endLine = 0;
	int endColumn = 0;
};

/// Всё, что нужно для отрисовки видимой области paged-редактора.
struct PagedTextPaintModel {
	QRect viewport;
	QPalette palette;
	QFont font;
	int firstLine = 0;
	int lineCount = 0;
	int horizontalScroll = 0;
	int rowOffsetPx = 0;
	int gutterWidth = 0;
	int tabWidth = TextLayout::kDefaultTabWidth;
	bool wrapped = false;
	bool lineNumbers = true;
	bool hasFocus = false;

	/// Курсоры и выделения многокурсорного режима.
	QVector<PagedTextSelection> selections;
	QVector<PagedTextCaret> carets;

	QString token;
	const QVector<PagedSearchMatch>* searchMatches = nullptr;
	int currentSearch = -1;

	std::function<QString(int)> rawLine;
	std::function<QString(int)> displayLine;
	std::function<QVector<PagedTextSpan>(int)> highlightSpans;
};

/// Отрисовка текста, выделения, поиска и gutter.
/// Не владеет состоянием и не знает о PagedTextEditor.
class PagedTextRenderer {
public:
	static void paint(QPainter& painter, const PagedTextPaintModel& model);
};
