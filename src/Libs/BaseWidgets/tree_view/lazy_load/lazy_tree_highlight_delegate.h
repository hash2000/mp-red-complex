#pragma once
#include <QStyledItemDelegate>

/// Делегат с подсветкой найденного текста.
///
/// Отрисовка полностью выполняется базовым QStyledItemDelegate (фон, иконка,
/// фокус, элидирование), поэтому выделение и тема оформления не ломаются.
/// Совпадения дополнительно тонируются цветом из палитры.
class HighlightDelegate : public QStyledItemDelegate {
	Q_OBJECT
public:
	explicit HighlightDelegate(const QString& highlightText, QObject* parent = nullptr);

	void setHighlightText(const QString& text);
	QString highlightText() const;

	void paint(QPainter* painter, const QStyleOptionViewItem& option,
		const QModelIndex& index) const override;

private:
	QString _highlightText;
};
