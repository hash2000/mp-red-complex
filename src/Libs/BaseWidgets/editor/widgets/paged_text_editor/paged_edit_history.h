#pragma once
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_document_buffer.h"

#include <QList>
#include <QPair>
#include <QStringList>
#include <QVector>

/// Позиция курсора и якоря выделения до или после правки.
struct PagedCursorState {
	int line = 0;
	int column = 0;
	int anchorLine = 0;
	int anchorColumn = 0;
};

/// Одна правка: снимки затронутых страниц до и после.
///
/// Снимок чистой страницы не содержит текста (см. PagedPageSnapshot), поэтому
/// отмена правки, выбросившей тысячи страниц, стоит несколько байт на страницу.
struct PagedEditRecord {
	QVector<QPair<int, PagedPageSnapshot>> before;
	QVector<QPair<int, PagedPageSnapshot>> after;
	/// Основной курсор (последний, к которому применялись действия).
	PagedCursorState cursorBefore;
	PagedCursorState cursorAfter;
	/// Остальные курсоры многокурсорного режима (без основного).
	QVector<PagedCursorState> extraCursorsBefore;
	QVector<PagedCursorState> extraCursorsAfter;
	/// Вид простой правки для склейки набора: 1 — вставка символа,
	/// 2 — Backspace, 3 — Delete. 0 — не склеивается.
	int mergeKind = 0;
	qint64 stamp = 0;
	qint64 cost = 0;
};

/// История правок с отменой и повтором.
///
/// Набор подряд идущих односимвольных правок склеивается в одну запись
/// (как в обычном редакторе): три набранных символа отменяются одним
/// нажатием, а не тремя. Склейка возможна только когда набор затронутых
/// страниц не меняется — иначе снимок «до» оказался бы неполным.
class PagedEditHistory {
public:
	static constexpr int kMaxRecords = 200;
	static constexpr qint64 kMaxCost = 64 * 1024 * 1024;
	static constexpr qint64 kMergeWindowMs = 1200;

	void clear();
	bool push(PagedEditRecord record, bool allowMerge);
	bool canUndo() const;
	bool canRedo() const;
	int undoCount() const;

	/// Забрать запись для отмены; она остаётся в истории (стек повтора).
	bool takeUndo(PagedEditRecord& record);
	/// Забрать запись для повтора.
	bool takeRedo(PagedEditRecord& record);

private:
	static qint64 costOf(const PagedEditRecord& record);
	static bool samePages(const PagedEditRecord& left, const PagedEditRecord& right);
	static bool samePosition(const PagedCursorState& left, const PagedCursorState& right);
	void trim();

	QList<PagedEditRecord> undoStack;
	QList<PagedEditRecord> redoStack;
	qint64 totalCost = 0;
};
