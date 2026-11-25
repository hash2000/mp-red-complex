#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_renderer.h"
#include "Libs/BaseWidgets/editor/layout/text_layout.h"

#include <QFontMetrics>
#include <QPainter>
#include <QtMath>

void PagedTextRenderer::paint(QPainter& painter, const PagedTextPaintModel& model) {
	const QFontMetrics metrics(model.font);
	const int lineHeight = qMax(1, metrics.height());
	const int gutter = model.gutterWidth;
	const int viewportWidth = model.viewport.width();
	const int viewportHeight = model.viewport.height();
	const int horizontalOffset = model.wrapped ? 0 : model.horizontalScroll;

	painter.fillRect(model.viewport, model.palette.base());
	if (gutter > 0) {
		painter.fillRect(QRect(0, 0, gutter, viewportHeight),
			model.palette.alternateBase());
		painter.setPen(model.palette.mid().color());
		painter.drawLine(gutter - 1, 0, gutter - 1, viewportHeight);
	}

	const QColor selectionColor = model.palette.highlight().color();
	const QColor tokenColor = model.palette.highlight().color();
	const QColor searchColor = model.palette.highlight().color();
	const QColor currentSearchColor = model.palette.highlight().color();

	painter.save();
	int y = -model.rowOffsetPx;
	for (int line = model.firstLine; line < model.lineCount && y < viewportHeight; line++) {
		const QString raw = model.rawLine ? model.rawLine(line) : QString();
		const QString text = model.displayLine ? model.displayLine(line) : raw;

		TextLayout layout(text, model.font, model.wrapped,
			qMax(1, viewportWidth - gutter - 8), TextLayout::kDefaultTabWidth);
		const int visualCount = qMax(1, layout.lineCount());
		bool firstVisual = true;

		for (int visual = 0; visual < visualCount && y < viewportHeight; visual++) {
			const QTextLine textLine = layout.lineAt(visual);
			const int visualHeight = textLine.isValid()
				? qMax(1, qCeil(textLine.height())) : lineHeight;
			if (!textLine.isValid()) {
				if (firstVisual && gutter > 0) {
					painter.setPen(model.palette.placeholderText().color());
					painter.drawText(QRectF(0, y, gutter - 8, visualHeight),
						Qt::AlignRight | Qt::AlignVCenter, QString::number(line + 1));
					firstVisual = false;
				}
				y += visualHeight;
				continue;
			}

			const int segmentStart = textLine.textStart();
			const int segmentEnd = segmentStart + textLine.textLength();
			const auto xAt = [&textLine](int column) {
				return textLine.cursorToX(column);
			};

			if (!model.token.isEmpty() && model.token.size() >= 2) {
				int position = text.indexOf(model.token, 0, Qt::CaseSensitive);
				while (position >= 0) {
					const int from = qMax(position, segmentStart);
					const int to = qMin(position + model.token.size(), segmentEnd);
					if (to > from) {
						QColor color = tokenColor;
						color.setAlpha(80);
						const int x1 = qRound(xAt(from));
						const int x2 = qRound(xAt(to));
						painter.fillRect(gutter - horizontalOffset + x1, y,
							x2 - x1, visualHeight, color);
					}
					position = text.indexOf(model.token, position + model.token.size(),
						Qt::CaseSensitive);
				}
			}

			for (int i = 0; model.searchMatches && i < model.searchMatches->size(); i++) {
				const auto& match = model.searchMatches->at(i);
				if (match.line != line) {
					continue;
				}
				const int from = qMax(match.start, segmentStart);
				const int to = qMin(match.start + match.length, segmentEnd);
				if (to <= from) {
					continue;
				}
				QColor color = i == model.currentSearch
					? currentSearchColor : searchColor;
				color.setAlpha(i == model.currentSearch ? 220 : 120);
				const int x1 = qRound(xAt(from));
				const int x2 = qRound(xAt(to));
				painter.fillRect(gutter - horizontalOffset + x1, y,
					x2 - x1, visualHeight, color);
			}

			if (model.hasSelection && line >= model.selectionStartLine
				&& line <= model.selectionEndLine) {
				const int selectedFrom = line == model.selectionStartLine
					? model.selectionStartColumn : 0;
				const int selectedTo = line == model.selectionEndLine
					? model.selectionEndColumn : text.size();
				const int from = qMax(selectedFrom, segmentStart);
				const int to = qMin(selectedTo, segmentEnd);
				if (to > from) {
					QColor color = selectionColor;
					color.setAlpha(170);
					const int x1 = qRound(xAt(from));
					const int x2 = qRound(xAt(to));
					painter.fillRect(gutter - horizontalOffset + x1, y,
						x2 - x1, visualHeight, color);
				}
			}

			painter.setPen(model.palette.text().color());
			textLine.draw(&painter, QPointF(gutter - horizontalOffset, y));

			if (model.hasFocus && line == model.cursorLine
				&& model.cursorColumn >= segmentStart
				&& (model.cursorColumn < segmentEnd
					|| (visual + 1 == visualCount
						&& model.cursorColumn == segmentEnd))) {
				const qreal cursorX = xAt(model.cursorColumn);
				painter.setPen(model.palette.text().color());
				painter.drawLine(
					QPointF(gutter - horizontalOffset + cursorX, y),
					QPointF(gutter - horizontalOffset + cursorX, y + visualHeight));
			}

			if (firstVisual && gutter > 0 && model.lineNumbers) {
				painter.setPen(model.palette.placeholderText().color());
				painter.drawText(QRectF(0, y, gutter - 8, visualHeight),
					Qt::AlignRight | Qt::AlignVCenter, QString::number(line + 1));
				firstVisual = false;
			}

			y += visualHeight;
		}
	}

	painter.restore();
}