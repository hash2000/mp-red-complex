#include "Libs/BaseWidgets/editor/layout/text_layout.h"

#include <QFontMetricsF>
#include <QTextOption>
#include <QtMath>

TextLayout::TextLayout(const QString& text, const QFont& font, bool wrapped,
	int width, int tabWidth, const QVector<QTextLayout::FormatRange>& formats)
	: layout(text, font) {
	QTextOption option;
	option.setWrapMode(wrapped ? QTextOption::WrapAtWordBoundaryOrAnywhere
		: QTextOption::NoWrap);
	option.setTabStopDistance(
		qMax<qreal>(1.0, QFontMetricsF(font).horizontalAdvance(QLatin1Char('M'))) * tabWidth);
	layout.setTextOption(option);
	layout.setFormats(formats);
	layout.beginLayout();
	while (true) {
		QTextLine line = layout.createLine();
		if (!line.isValid()) {
			break;
		}
		line.setLineWidth(wrapped ? qMax(1, width) : 1e9);
		if (!wrapped) {
			break;
		}
	}
	layout.endLayout();
}

int TextLayout::lineCount() const {
	return layout.lineCount();
}

QTextLine TextLayout::lineAt(int visualRow) const {
	return layout.lineAt(visualRow);
}

int TextLayout::visualRowForColumn(int column) const {
	int visual = 0;
	int y = 0;
	int height = 0;
	return rowForColumn(column, visual, y, height) ? visual : 0;
}

bool TextLayout::rowForColumn(int column, int& visualRow, int& y, int& height) const {
	int top = 0;
	for (int visual = 0; visual < layout.lineCount(); visual++) {
		const QTextLine textLine = layout.lineAt(visual);
		const int rowHeight = qMax(1, qCeil(textLine.height()));
		const int end = textLine.textStart() + textLine.textLength();
		if (column >= textLine.textStart()
			&& (column < end
				|| (visual + 1 == layout.lineCount() && column == end))) {
			visualRow = visual;
			y = top;
			height = rowHeight;
			return true;
		}
		top += rowHeight;
	}
	return false;
}

bool TextLayout::rowAtY(int y, int& visualRow, int& top, int& height) const {
	int current = 0;
	for (int visual = 0; visual < layout.lineCount(); visual++) {
		const QTextLine textLine = layout.lineAt(visual);
		const int rowHeight = qMax(1, qCeil(textLine.height()));
		if (y < current + rowHeight) {
			visualRow = visual;
			top = current;
			height = rowHeight;
			return true;
		}
		current += rowHeight;
	}
	return false;
}

qreal TextLayout::xForColumn(int column) const {
	int visual = 0;
	int y = 0;
	int height = 0;
	if (!rowForColumn(column, visual, y, height)) {
		return 0;
	}
	return layout.lineAt(visual).cursorToX(column);
}

int TextLayout::columnAt(int visualRow, qreal x) const {
	if (visualRow < 0 || visualRow >= layout.lineCount()) {
		return 0;
	}
	return layout.lineAt(visualRow).xToCursor(x);
}

qreal TextLayout::naturalWidth(int visualRow) const {
	if (visualRow < 0 || visualRow >= layout.lineCount()) {
		return 0;
	}
	return layout.lineAt(visualRow).naturalTextWidth();
}

void TextLayout::setFormats(const QVector<QTextLayout::FormatRange>& formats) {
	layout.setFormats(formats);
}
