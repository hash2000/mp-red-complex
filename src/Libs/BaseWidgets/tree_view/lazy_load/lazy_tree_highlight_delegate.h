#pragma once
#include <QStyledItemDelegate>

class HighlightDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit HighlightDelegate(const QString& highlightText, QObject* parent = nullptr);

	void setHighlightText(const QString& text);
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

private:
	QString _highlightText;
};
