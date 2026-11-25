#pragma once
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_search.h"

#include <QFont>
#include <QHash>
#include <QPalette>
#include <QRect>
#include <QString>
#include <QVector>

#include <functional>

class QPainter;

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
	bool wrapped = false;
	bool lineNumbers = true;
	bool hasFocus = false;

	int cursorLine = 0;
	int cursorColumn = 0;
	bool hasSelection = false;
	int selectionStartLine = 0;
	int selectionStartColumn = 0;
	int selectionEndLine = 0;
	int selectionEndColumn = 0;

	QString token;
	const QVector<PagedSearchMatch>* searchMatches = nullptr;
	int currentSearch = -1;

	std::function<QString(int)> rawLine;
	std::function<QString(int)> displayLine;
};

/// Отрисовка текста, выделения, поиска и gutter.
/// Не владеет состоянием и не знает о PagedTextEditor.
class PagedTextRenderer {
public:
	static void paint(QPainter& painter, const PagedTextPaintModel& model);
};
