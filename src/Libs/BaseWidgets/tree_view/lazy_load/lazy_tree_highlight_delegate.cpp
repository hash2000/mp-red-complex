#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_highlight_delegate.h"

#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QStyle>

HighlightDelegate::HighlightDelegate(const QString& highlightText, QObject* parent)
	: QStyledItemDelegate(parent)
	, _highlightText(highlightText) {
}

void HighlightDelegate::setHighlightText(const QString& text) {
	_highlightText = text;
}

QString HighlightDelegate::highlightText() const {
	return _highlightText;
}

void HighlightDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
	const QModelIndex& index) const {
	// Базовая отрисовка сохраняет выделение, фокус, иконки и элидирование.
	QStyledItemDelegate::paint(painter, option, index);

	if (_highlightText.isEmpty()) {
		return;
	}

	QStyleOptionViewItem opt = option;
	initStyleOption(&opt, index);

	const QString text = opt.text;
	if (text.isEmpty()) {
		return;
	}

	QStyle* style = opt.widget ? opt.widget->style() : QApplication::style();
	const QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &opt, opt.widget);
	if (!textRect.isValid() || textRect.isEmpty()) {
		return;
	}

	QColor highlight = opt.palette.color(QPalette::Highlight);
	highlight.setAlpha(80);

	const QFontMetrics metrics(opt.font);
	painter->save();
	painter->setClipRect(textRect);

	int from = 0;
	while (from < text.size()) {
		const int position = text.indexOf(_highlightText, from, Qt::CaseInsensitive);
		if (position < 0) {
			break;
		}

		const int left = textRect.x() + metrics.horizontalAdvance(text.left(position));
		const int width = metrics.horizontalAdvance(text.mid(position, _highlightText.size()));
		painter->fillRect(QRect(left, textRect.y(), width, textRect.height()), highlight);

		from = position + _highlightText.size();
	}

	painter->restore();
}
