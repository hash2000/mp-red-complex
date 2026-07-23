#include "Libs/BaseWidgets/tree_view/lazy_load/lazy_tree_highlight_delegate.h"
#include <QPainter>

HighlightDelegate::HighlightDelegate(const QString& highlightText, QObject* parent)
	: QStyledItemDelegate(parent)
	, _highlightText(highlightText) {}

void HighlightDelegate::setHighlightText(const QString& text) {
	_highlightText = text;
}

void HighlightDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const {
	QStyleOptionViewItem opt = option;
	initStyleOption(&opt, index);

	QString text = opt.text;
	painter->save();

	// Рисуем фон выделения (если строка выбрана)
	if (opt.state & QStyle::State_Selected) {
		painter->fillRect(opt.rect, opt.palette.highlight());
		painter->setPen(opt.palette.highlightedText().color());
	}
	else {
		painter->setPen(opt.palette.text().color());
	}

	// Логика подсветки
	int highlightStart = text.indexOf(_highlightText, 0, Qt::CaseInsensitive);
	if (highlightStart >= 0 && !_highlightText.isEmpty()) {
		int highlightLength = _highlightText.length();

		// Рисуем обычный текст до совпадения
		QString preText = text.left(highlightStart);
		QFontMetrics fm(opt.font);
		int preWidth = fm.horizontalAdvance(preText);
		painter->drawText(opt.rect.x(), opt.rect.y() + fm.ascent(), preText);

		// Рисуем подсвеченный текст
		QRect highlightRect(opt.rect.x() + preWidth, opt.rect.y(), fm.horizontalAdvance(text.mid(highlightStart, highlightLength)), opt.rect.height());
		painter->fillRect(highlightRect, QColor(255, 255, 0, 100)); // Полупрозрачный желтый
		painter->drawText(highlightRect, Qt::AlignLeft | Qt::AlignVCenter, text.mid(highlightStart, highlightLength));

		// Рисуем остаток текста
		int postX = highlightRect.right() + 1;
		painter->drawText(postX, opt.rect.y() + fm.ascent(), text.mid(highlightStart + highlightLength));
	}
	else {
		// Без подсветки
		painter->drawText(opt.rect, Qt::AlignLeft | Qt::AlignVCenter, text);
	}

	painter->restore();
}
