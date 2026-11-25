#include "Libs/BaseWidgets/editor/widgets/paged_text_editor.h"
#include "Libs/Base/background_task_registry.h"
#include "Libs/BaseWidgets/editor/formatters/formatter_plugin.h"
#include "Libs/BaseWidgets/editor/formatters/formatter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin_manager.h"
#include "Libs/BaseWidgets/editor/language_info.h"
#include "Libs/BaseWidgets/editor/layout/text_layout.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_document_buffer.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_edit_history.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_file_indexer.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_line_chunks.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_search.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_cursor.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_highlighter.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_renderer.h"

#include <QAction>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFile>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QHash>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSaveFile>
#include <QTextCodec>
#include <QTextLine>
#include <QTimer>
#include <QWheelEvent>
#include <QtConcurrent/QtConcurrentRun>
#include <QtMath>

#include <atomic>
#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

namespace {

/// Дополнительный отступ справа, чтобы последний символ не упирался в край.
constexpr int kHorizontalMarginPx = 24;

/// Виды простых правок, которые история склеивает в один шаг отмены.
constexpr int kMergeInsert = 1;
constexpr int kMergeBackspace = 2;
constexpr int kMergeDelete = 3;

/// Задержка перезапуска поиска после правки: не считаем весь файл на
/// каждое нажатие клавиши.
constexpr int kSearchRefreshDelayMs = 150;

} // namespace

struct PagedTextEditor::Private {
	PagedTextEditor* q = nullptr;

	QString path;
	QString encodingName = QStringLiteral("UTF-8");
	QTextCodec* codec = nullptr;

	bool indexing = false;
	bool ready = false;
	bool wrapLines = false;
	bool showLineNumbers = true;
	bool readOnly = false;
	QString error;

	int lines = 0;
	int maxLineLength = 0;
	qint64 fileSize = 0;
	QVector<qint64> lineStarts;

	PagedDocumentBuffer buffer;
	PagedEditHistory history;
	PagedLineChunks chunks;
	PagedTextHighlighter highlighter;
	bool highlightingEnabled = true;
	HighlightingPluginManager* highlightingManager = nullptr;
	FormatterPluginManager* formatterManager = nullptr;
	FormatterPlugin* currentFormatter = nullptr;
	QString formatSource;

	bool modified = false;
	bool undoAvailable = false;
	bool redoAvailable = false;
	int spilledPages = 0;

	QFutureWatcher<PagedIndexResult>* watcher = nullptr;
	std::shared_ptr<std::atomic_bool> cancelFlag;
	quint64 generation = 0;
	quint64 activeGeneration = 0;

	int cursorLine = 0;
	int cursorColumn = 0;
	int anchorLine = 0;
	int anchorColumn = 0;
	bool selecting = false;
	int restoreLine = -1;
	int restoreCursorLine = -1;
	int restoreCursorColumn = 0;

	QString searchText;
	QVector<PagedSearchMatch> searchMatches;
	int searchTotal = 0;
	int searchCurrent = -1;
	bool searching = false;
	bool searchSelectionActive = false;
	bool searchCaseSensitive = false;
	bool searchJumpOnFinish = true;
	int rowOffsetPx = 0;
	bool loadingMoreHorizontally = false;
	QFutureWatcher<PagedSearchOutcome>* searchWatcher = nullptr;
	std::shared_ptr<std::atomic_bool> searchCancel;
	quint64 searchGeneration = 0;
	quint64 searchActiveGeneration = 0;
	QTimer* searchRefreshTimer = nullptr;

	void clearPages();
	QString indexTaskId() const;
	QString searchTaskId() const;
	QString formatTaskId() const;
	QString lineAt(int line) const;
	QString visibleText(int line) const;
	int lineLimit(int line) const;
	int maxLineLimit() const;
	void maybeLoadMoreHorizontally();
	void updateScrollBars();

	int lineHeight() const;
	bool hasSelection() const;
	void normalizedSelection(int& startLine, int& startColumn,
		int& endLine, int& endColumn) const;
	QString selectedToken() const;
	int gutterWidth() const;
	int logicalLineAtY(int y) const;
	bool positionAt(int x, int y, int& line, int& column) const;
	void ensureCursorVisible();
	bool cursorRowGeometry(int line, int& y, int& height) const;
	void selectMatch(int index);
	void updateCurrentForCursor();
	void moveCursorVertically(int delta, bool keepAnchor);

	// Редактирование.
	bool canEdit() const;
	void resetEditState();
	bool replaceRange(int startLine, int startColumn, int endLine, int endColumn,
		const QString& text, int mergeKind);
	void syncModel();
	void afterEdit();
	void applyHistoryState(const PagedCursorState& cursor);
	void setCursorFromState(const PagedCursorState& cursor);
	void backspace();
	void deleteForward();
	void startSearch(const QString& query, bool jump);
	void scheduleSearchRefresh();
	void syncSpillState();
};

void PagedTextEditor::Private::clearPages() {
	buffer.clear();
}

QString PagedTextEditor::Private::indexTaskId() const {
	return QStringLiteral("paged-index:%1").arg(quintptr(q), 0, 16);
}

QString PagedTextEditor::Private::searchTaskId() const {
	return QStringLiteral("paged-search:%1").arg(quintptr(q), 0, 16);
}

QString PagedTextEditor::Private::formatTaskId() const {
	return QStringLiteral("paged-format:%1").arg(quintptr(q), 0, 16);
}

QString PagedTextEditor::Private::lineAt(int line) const {
	if (!ready || line < 0 || line >= lines) {
		return QString();
	}
	return buffer.lineAt(line);
}

QString PagedTextEditor::Private::visibleText(int line) const {
	return chunks.displayedText(lineAt(line), line, wrapLines, maxLineLength);
}

int PagedTextEditor::Private::lineLimit(int line) const {
	return chunks.limit(line, wrapLines, maxLineLength);
}

int PagedTextEditor::Private::maxLineLimit() const {
	return chunks.maximumLimit(wrapLines, maxLineLength);
}

void PagedTextEditor::Private::maybeLoadMoreHorizontally() {
	if (!ready || wrapLines || loadingMoreHorizontally) {
		return;
	}

	auto* horizontal = q->horizontalScrollBar();
	const int threshold = qMax(1, q->fontMetrics().horizontalAdvance(QLatin1Char('W'))) * 2;
	if (horizontal->maximum() <= 0
		|| horizontal->value() < horizontal->maximum() - threshold) {
		return;
	}

	loadingMoreHorizontally = true;
	bool loaded = false;
	const int first = q->verticalScrollBar()->value();
	const int visibleLines = qMax(1, q->verticalScrollBar()->pageStep());
	for (int i = first; i < lines && i < first + visibleLines; i++) {
		if (lineLimit(i) < lineAt(i).size()) {
			chunks.loadMore(i);
			loaded = true;
		}
	}
	loadingMoreHorizontally = false;

	if (loaded) {
		updateScrollBars();
		q->viewport()->update();
	}
}

int PagedTextEditor::Private::lineHeight() const {
	return qMax(1, q->fontMetrics().height());
}

bool PagedTextEditor::Private::hasSelection() const {
	return cursorLine != anchorLine || cursorColumn != anchorColumn;
}

void PagedTextEditor::Private::normalizedSelection(int& startLine, int& startColumn,
	int& endLine, int& endColumn) const {
	if (cursorLine < anchorLine
		|| (cursorLine == anchorLine && cursorColumn < anchorColumn)) {
		startLine = cursorLine;
		startColumn = cursorColumn;
		endLine = anchorLine;
		endColumn = anchorColumn;
		return;
	}

	startLine = anchorLine;
	startColumn = anchorColumn;
	endLine = cursorLine;
	endColumn = cursorColumn;
}

QString PagedTextEditor::Private::selectedToken() const {
	if (!hasSelection()) {
		return QString();
	}

	int startLine = 0;
	int startColumn = 0;
	int endLine = 0;
	int endColumn = 0;
	normalizedSelection(startLine, startColumn, endLine, endColumn);
	if (startLine != endLine || endColumn <= startColumn
		|| endColumn - startColumn > 128) {
		return QString();
	}

	const QString text = lineAt(startLine).mid(startColumn,
		endColumn - startColumn);
	for (const QChar character : text) {
		if (!character.isLetterOrNumber() && character != QLatin1Char('_')) {
			return QString();
		}
	}
	return text;
}

int PagedTextEditor::Private::gutterWidth() const {
	if (!showLineNumbers) {
		return 0;
	}

	const int digits = qMax(2, QString::number(qMax(1, lines)).size());
	return q->fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits + 14;
}

int PagedTextEditor::Private::logicalLineAtY(int y) const {
	if (!ready || lines <= 0) {
		return 0;
	}

	const int first = q->verticalScrollBar()->value();
	const int layoutWidth = qMax(1, q->viewport()->width() - gutterWidth() - 8);
	int top = -rowOffsetPx;
	for (int i = first; i < lines; i++) {
		TextLayout layout(visibleText(i), q->font(), wrapLines, layoutWidth, TextLayout::kDefaultTabWidth);
		const int count = qMax(1, layout.lineCount());
		for (int visual = 0; visual < count; visual++) {
			const QTextLine textLine = layout.lineAt(visual);
			const int height = textLine.isValid()
				? qMax(1, qCeil(textLine.height())) : q->fontMetrics().height();
			if (y < top + height) {
				return i;
			}
			top += height;
		}
		if (top > q->viewport()->height()) {
			break;
		}
	}
	return qMax(0, lines - 1);
}

bool PagedTextEditor::Private::positionAt(int x, int y, int& line, int& column) const {
	if (!ready || lines <= 0) {
		return false;
	}

	const int first = q->verticalScrollBar()->value();
	const int gutter = gutterWidth();
	const qreal contentX = x - gutter
		+ (wrapLines ? 0 : q->horizontalScrollBar()->value());
	const int layoutWidth = qMax(1, q->viewport()->width() - gutter - 8);
	int top = -rowOffsetPx;
	for (int i = first; i < lines; i++) {
		const QString text = visibleText(i);
		TextLayout layout(text, q->font(), wrapLines, layoutWidth, TextLayout::kDefaultTabWidth);
		int visual = 0;
		int rowTop = 0;
		int rowHeight = 0;
		if (layout.rowAtY(y - top, visual, rowTop, rowHeight)) {
			line = i;
			column = qBound(0, layout.columnAt(visual, contentX), text.size());
			return true;
		}
		const int count = qMax(1, layout.lineCount());
		for (int row = 0; row < count; row++) {
			const QTextLine textLine = layout.lineAt(row);
			top += textLine.isValid()
				? qMax(1, qCeil(textLine.height())) : q->fontMetrics().height();
		}
		if (top > q->viewport()->height()) {
			break;
		}
	}
	return false;
}

void PagedTextEditor::Private::ensureCursorVisible() {
	if (!ready || lines <= 0) {
		return;
	}

	auto* vertical = q->verticalScrollBar();
	int cursorY = 0;
	int cursorHeight = 0;
	if (!cursorRowGeometry(cursorLine, cursorY, cursorHeight)) {
		return;
	}

	const int viewportHeight = q->viewport()->height();
	const int pageStep = qMax(1, vertical->pageStep());
	if (cursorLine < vertical->value()
		|| cursorLine >= vertical->value() + pageStep) {
		// Далёкий переход не считаем построчно: сразу ставим строку
		// началом области и при необходимости прокручиваем внутри неё.
		vertical->setValue(cursorLine);
		rowOffsetPx = qMax(0, cursorY + cursorHeight - viewportHeight);
		if (cursorY < rowOffsetPx) {
			rowOffsetPx = cursorY;
		}
		return;
	}

	// Экранная позиция строки курсора относительно текущей прокрутки.
	int visibleY = -rowOffsetPx + cursorY;
	int previousHeight = 0;
	const int layoutWidth = qMax(1, q->viewport()->width() - gutterWidth() - 8);
	for (int i = vertical->value(); i < cursorLine; i++) {
		TextLayout layout(visibleText(i), q->font(), wrapLines, layoutWidth, TextLayout::kDefaultTabWidth);
		for (int visual = 0; visual < layout.lineCount(); visual++) {
			previousHeight += qMax(1, qCeil(layout.lineAt(visual).height()));
		}
	}
	visibleY += previousHeight;

	if (visibleY >= 0 && visibleY + cursorHeight <= viewportHeight) {
		return;
	}

	// Строку с найденным фрагментом ставим началом видимой области, а
	// внутри неё оставляем нужную экранную строку.
	vertical->setValue(cursorLine);
	rowOffsetPx = qMax(0, cursorY + cursorHeight - viewportHeight);
	if (cursorY < rowOffsetPx) {
		rowOffsetPx = cursorY;
	}
}

bool PagedTextEditor::Private::cursorRowGeometry(int line, int& y, int& height) const {
	if (line < 0 || line >= lines) {
		return false;
	}

	const QString text = visibleText(line);
	const int column = qMin(cursorColumn, text.size());
	const int layoutWidth = qMax(1, q->viewport()->width() - gutterWidth() - 8);
	TextLayout layout(text, q->font(), wrapLines, layoutWidth, TextLayout::kDefaultTabWidth);
	int visual = 0;
	return layout.rowForColumn(column, visual, y, height);
}

void PagedTextEditor::Private::moveCursorVertically(int delta, bool keepAnchor) {
	if (!ready || lines <= 0) {
		return;
	}

	const int layoutWidth = qMax(1, q->viewport()->width() - gutterWidth() - 8);
	const PagedTextPosition target = PagedTextCursor::moveVertical(
		{ cursorLine, cursorColumn }, delta,
		[this](int line) { return visibleText(line); },
		q->font(), wrapLines, layoutWidth, lines);
	if (target.line != cursorLine || target.column != cursorColumn) {
		q->setCursorPosition(target.line, target.column, keepAnchor);
	}
}

void PagedTextEditor::Private::selectMatch(int index) {
	if (index < 0 || index >= searchMatches.size()) {
		return;
	}

	searchCurrent = index;
	searchSelectionActive = true;
	const auto& match = searchMatches.at(index);
	// Если совпадение в ещё не загруженном куске длинной строки — догружаем.
	chunks.ensureLimit(match.line, match.start + match.length,
		wrapLines, maxLineLength);
	cursorLine = match.line;
	cursorColumn = match.start;
	anchorLine = match.line;
	anchorColumn = match.start + match.length;
	ensureCursorVisible();
	q->viewport()->update();
	emit q->cursorPositionChanged(cursorLine, cursorColumn);
	emit q->searchCurrentChanged(searchCurrent, searchTotal);
}

void PagedTextEditor::Private::updateCurrentForCursor() {
	if (searchMatches.isEmpty()) {
		if (searchCurrent != -1) {
			searchCurrent = -1;
			emit q->searchCurrentChanged(-1, 0);
		}
		return;
	}

	int index = -1;
	for (int i = 0; i < searchMatches.size(); i++) {
		const auto& match = searchMatches.at(i);
		// Курсор внутри совпадения или перед ним — это текущее совпадение.
		if (match.line > cursorLine
			|| (match.line == cursorLine
				&& match.start + match.length > cursorColumn)) {
			index = i;
			break;
		}
	}
	if (index < 0) {
		index = 0;
	}

	if (searchCurrent != index) {
		searchCurrent = index;
		emit q->searchCurrentChanged(searchCurrent, searchTotal);
	}
}

void PagedTextEditor::Private::updateScrollBars() {
	if (!q) {
		return;
	}

	const int lineHeight = qMax(1, q->fontMetrics().height());
	const int pageStep = qMax(1, q->viewport()->height() / lineHeight);

	auto* vertical = q->verticalScrollBar();
	vertical->setRange(0, qMax(0, lines - 1));
	vertical->setPageStep(pageStep);
	vertical->setSingleStep(1);

	const int charWidth = qMax(1, q->fontMetrics().horizontalAdvance(QLatin1Char('W')));
	const qint64 contentWidth = static_cast<qint64>(
		qMin(maxLineLength, maxLineLimit())) * charWidth + kHorizontalMarginPx;
	const int availableWidth = qMax(1, q->viewport()->width() - gutterWidth());
	const int maxHorizontal = static_cast<int>(qMin<qint64>(
		qMax<qint64>(0, contentWidth - availableWidth),
		std::numeric_limits<int>::max()));
	auto* horizontal = q->horizontalScrollBar();
	if (wrapLines) {
		horizontal->setRange(0, 0);
		horizontal->setPageStep(0);
		return;
	}
	horizontal->setRange(0, maxHorizontal);
	horizontal->setPageStep(q->viewport()->width());
	horizontal->setSingleStep(qMax(1, charWidth));
}

bool PagedTextEditor::Private::canEdit() const {
	return ready && !indexing && !readOnly;
}

void PagedTextEditor::Private::resetEditState() {
	if (modified) {
		modified = false;
		emit q->modifiedChanged(false);
	}
	if (undoAvailable) {
		undoAvailable = false;
		emit q->undoAvailableChanged(false);
	}
	if (redoAvailable) {
		redoAvailable = false;
		emit q->redoAvailableChanged(false);
	}
}

void PagedTextEditor::Private::syncSpillState() {
	const int spilled = buffer.spilledPageCount();
	if (spilled == spilledPages) {
		return;
	}
	spilledPages = spilled;
	emit q->bufferSpilled(spilled);
}

void PagedTextEditor::Private::syncModel() {
	const int newLines = buffer.lineCount();
	const bool lineCountChanged = newLines != lines;
	lines = newLines;
	maxLineLength = buffer.maxLineLength();
	if (lineCountChanged) {
		chunks.clear();
	}

	updateScrollBars();
	q->viewport()->update();
	if (lineCountChanged) {
		emit q->lineCountChanged(lines);
	}

	const bool newModified = buffer.hasChanges();
	if (newModified != modified) {
		modified = newModified;
		emit q->modifiedChanged(modified);
	}

	const bool newUndo = history.canUndo();
	if (newUndo != undoAvailable) {
		undoAvailable = newUndo;
		emit q->undoAvailableChanged(newUndo);
	}

	const bool newRedo = history.canRedo();
	if (newRedo != redoAvailable) {
		redoAvailable = newRedo;
		emit q->redoAvailableChanged(newRedo);
	}

	syncSpillState();
}

void PagedTextEditor::Private::afterEdit() {
	syncModel();
	ensureCursorVisible();
	emit q->cursorPositionChanged(cursorLine, cursorColumn);
	scheduleSearchRefresh();
}

void PagedTextEditor::Private::setCursorFromState(const PagedCursorState& state) {
	lines = buffer.lineCount();
	cursorLine = qBound(0, state.line, qMax(0, lines - 1));
	cursorColumn = qBound(0, state.column, lineAt(cursorLine).size());
	anchorLine = qBound(0, state.anchorLine, qMax(0, lines - 1));
	anchorColumn = qBound(0, state.anchorColumn, lineAt(anchorLine).size());
	searchSelectionActive = false;
}

void PagedTextEditor::Private::applyHistoryState(const PagedCursorState& cursor) {
	const int previousLines = lines;
	maxLineLength = buffer.maxLineLength();
	chunks.clear();
	setCursorFromState(cursor);
	updateScrollBars();
	ensureCursorVisible();
	q->viewport()->update();

	if (lines != previousLines) {
		emit q->lineCountChanged(lines);
	}

	const bool newModified = buffer.hasChanges();
	if (newModified != modified) {
		modified = newModified;
		emit q->modifiedChanged(modified);
	}

	const bool newUndo = history.canUndo();
	if (newUndo != undoAvailable) {
		undoAvailable = newUndo;
		emit q->undoAvailableChanged(newUndo);
	}

	const bool newRedo = history.canRedo();
	if (newRedo != redoAvailable) {
		redoAvailable = newRedo;
		emit q->redoAvailableChanged(newRedo);
	}

	emit q->cursorPositionChanged(cursorLine, cursorColumn);
	syncSpillState();
	scheduleSearchRefresh();
}

bool PagedTextEditor::Private::replaceRange(int startLine, int startColumn,
	int endLine, int endColumn, const QString& text, int mergeKind) {
	if (!canEdit()) {
		return false;
	}

	if (startLine > endLine || (startLine == endLine && startColumn > endColumn)) {
		std::swap(startLine, endLine);
		std::swap(startColumn, endColumn);
	}

	const QVector<int> pages = buffer.pagesForLines(startLine, endLine);
	if (pages.isEmpty()) {
		return false;
	}

	PagedEditRecord record;
	record.before.reserve(pages.size());
	for (const int page : pages) {
		record.before.append({ page, buffer.pageSnapshot(page) });
	}
	record.cursorBefore = { cursorLine, cursorColumn, anchorLine, anchorColumn };

	if (!buffer.replace(startLine, startColumn, endLine, endColumn, text)) {
		return false;
	}

	record.after.reserve(pages.size());
	for (const int page : pages) {
		record.after.append({ page, buffer.pageSnapshot(page) });
	}

	// Курсор встаёт сразу за вставленным текстом.
	const QStringList inserted = text.isEmpty()
		? QStringList{ QString() } : text.split(QLatin1Char('\n'));
	const int parts = inserted.size();
	cursorLine = qBound(0, startLine + parts - 1, qMax(0, buffer.lineCount() - 1));
	const int newColumn = parts == 1
		? startColumn + inserted.first().size() : inserted.last().size();
	cursorColumn = qBound(0, newColumn, buffer.lineAt(cursorLine).size());
	anchorLine = cursorLine;
	anchorColumn = cursorColumn;
	searchSelectionActive = false;

	record.cursorAfter = { cursorLine, cursorColumn, anchorLine, anchorColumn };
	record.mergeKind = mergeKind;
	record.stamp = QDateTime::currentMSecsSinceEpoch();

	// Склейка нужна только для набора текста и посимвольного удаления.
	history.push(record, mergeKind != 0);

	if (text.contains(QLatin1Char('\n')) || startLine != endLine) {
		chunks.clear();
	}

	afterEdit();
	return true;
}

void PagedTextEditor::Private::backspace() {
	if (!canEdit()) {
		return;
	}

	if (hasSelection()) {
		q->deleteSelection();
		return;
	}

	if (cursorColumn > 0) {
		replaceRange(cursorLine, cursorColumn - 1, cursorLine, cursorColumn,
			QString(), kMergeBackspace);
		return;
	}

	if (cursorLine > 0) {
		const int previousLength = lineAt(cursorLine - 1).size();
		replaceRange(cursorLine - 1, previousLength, cursorLine, 0,
			QString(), kMergeBackspace);
	}
}

void PagedTextEditor::Private::deleteForward() {
	if (!canEdit()) {
		return;
	}

	if (hasSelection()) {
		q->deleteSelection();
		return;
	}

	const int lineLength = lineAt(cursorLine).size();
	if (cursorColumn < lineLength) {
		replaceRange(cursorLine, cursorColumn, cursorLine, cursorColumn + 1,
			QString(), kMergeDelete);
		return;
	}

	if (cursorLine + 1 < lines) {
		replaceRange(cursorLine, cursorColumn, cursorLine + 1, 0,
			QString(), kMergeDelete);
	}
}

void PagedTextEditor::Private::scheduleSearchRefresh() {
	if (searchText.isEmpty() || !searchRefreshTimer) {
		return;
	}
	searchRefreshTimer->start();
}

void PagedTextEditor::Private::startSearch(const QString& query, bool jump) {
	searchText = query;
	searchJumpOnFinish = jump;
	searchGeneration++;
	if (searchRefreshTimer) {
		searchRefreshTimer->stop();
	}
	if (searchCancel) {
		searchCancel->store(true);
	}
	searching = false;
	searchMatches.clear();
	searchTotal = 0;
	searchCurrent = -1;
	emit q->searchCurrentChanged(-1, 0);
	q->viewport()->update();

	if (!ready || query.isEmpty()) {
		BackgroundTaskRegistry::instance().endTask(searchTaskId());
		emit q->searchCountChanged(0);
		return;
	}

	emit q->searchCountChanged(-1);
	BackgroundTaskRegistry::instance().beginTask(searchTaskId(),
		PagedTextEditor::tr("Поиск: %1").arg(query));
	auto cancel = std::make_shared<std::atomic_bool>(false);
	searchCancel = cancel;
	searchActiveGeneration = searchGeneration;
	searching = true;

	// Снимок буфера: изменённые страницы берутся из памяти, неизменённые
	// по-прежнему читаются с диска. Копия разделяет строки (copy-on-write),
	// поэтому стоит недорого.
	const PagedDocumentBuffer snapshot = buffer;
	const Qt::CaseSensitivity caseSensitivity = searchCaseSensitive
		? Qt::CaseSensitive : Qt::CaseInsensitive;
	PagedTextEditor* widget = q;
	searchWatcher->setFuture(QtConcurrent::run(
		[snapshot, query, caseSensitivity, cancel, widget,
			generation = searchGeneration] {
			return runPagedLineSearch(snapshot, query, caseSensitivity, cancel,
				[widget, generation](int total) {
					QMetaObject::invokeMethod(widget,
						[widget, generation, total] {
							if (widget->d
								&& generation == widget->d->searchGeneration) {
								emit widget->searchProgressChanged(total);
							}
						}, Qt::QueuedConnection);
				});
		}));
}

PagedTextEditor::PagedTextEditor(QWidget* parent)
	: QAbstractScrollArea(parent)
	, d(std::make_unique<Private>()) {
	d->q = this;

	setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	setFocusPolicy(Qt::StrongFocus);
	setFrameShape(QFrame::NoFrame);
	viewport()->setAttribute(Qt::WA_OpaquePaintEvent, true);

	d->watcher = new QFutureWatcher<PagedIndexResult>(this);
	connect(d->watcher, &QFutureWatcher<PagedIndexResult>::finished, this, [this] {
		if (!d) {
			return;
		}

		BackgroundTaskRegistry::instance().endTask(d->indexTaskId());
		d->indexing = false;
		const PagedIndexResult result = d->watcher->result();
		if (d->activeGeneration != d->generation) {
			return;
		}

		if (!result.ok || result.cancelled) {
			d->ready = false;
			d->error = result.cancelled ? QString() : result.error;
			viewport()->update();
			if (!result.cancelled) {
				emit indexingFinished(false, d->error);
			}
			return;
		}

		d->lineStarts = result.lineStarts;
		d->lines = result.lineStarts.size();
		d->maxLineLength = result.maxLineLength;
		d->fileSize = result.fileSize;
		d->buffer.reset(d->path, d->encodingName.toLatin1(),
			d->lineStarts, d->fileSize, d->maxLineLength);
		d->ready = true;
		d->error.clear();
		d->chunks.clear();
		d->updateScrollBars();
		verticalScrollBar()->setValue(d->restoreLine >= 0
			? qMin(d->restoreLine, qMax(0, d->lines - 1)) : 0);
		d->restoreLine = -1;
		if (d->restoreCursorLine >= 0) {
			d->cursorLine = qBound(0, d->restoreCursorLine, qMax(0, d->lines - 1));
			d->cursorColumn = qBound(0, d->restoreCursorColumn,
				d->lineAt(d->cursorLine).size());
			d->anchorLine = d->cursorLine;
			d->anchorColumn = d->cursorColumn;
			d->restoreCursorLine = -1;
			d->restoreCursorColumn = 0;
		}
		else {
			d->cursorLine = 0;
			d->cursorColumn = 0;
			d->anchorLine = 0;
			d->anchorColumn = 0;
		}
		viewport()->update();
		emit lineCountChanged(d->lines);
		emit cursorPositionChanged(d->cursorLine, d->cursorColumn);
		emit indexingFinished(true, QString());
	});

	d->searchWatcher = new QFutureWatcher<PagedSearchOutcome>(this);
	connect(d->searchWatcher, &QFutureWatcher<PagedSearchOutcome>::finished, this, [this] {
		if (!d) {
			return;
		}
		BackgroundTaskRegistry::instance().endTask(d->searchTaskId());
		d->searching = false;
		const PagedSearchOutcome outcome = d->searchWatcher->result();
		if (d->searchActiveGeneration != d->searchGeneration || outcome.cancelled) {
			return;
		}

		d->searchMatches = outcome.matches;
		d->searchTotal = outcome.total;
		d->searchCurrent = -1;
		if (!d->searchMatches.isEmpty()) {
			if (d->searchJumpOnFinish) {
				int index = 0;
				for (int i = 0; i < d->searchMatches.size(); i++) {
					const auto& match = d->searchMatches.at(i);
					if (match.line > d->cursorLine
						|| (match.line == d->cursorLine && match.start >= d->cursorColumn)) {
						index = i;
						break;
					}
				}
				d->selectMatch(index);
			}
			else {
				// Поиск перезапущен после правки: курсор не переводим.
				d->updateCurrentForCursor();
			}
		}

		viewport()->update();
		emit searchCountChanged(d->searchTotal);
	});

	d->searchRefreshTimer = new QTimer(this);
	d->searchRefreshTimer->setSingleShot(true);
	d->searchRefreshTimer->setInterval(kSearchRefreshDelayMs);
	connect(d->searchRefreshTimer, &QTimer::timeout, this, [this] {
		if (!d || d->searchText.isEmpty()) {
			return;
		}
		d->startSearch(d->searchText, false);
	});

	connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
		d->rowOffsetPx = 0;
	});
	connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] {
		d->maybeLoadMoreHorizontally();
	});

	d->updateScrollBars();

	// Редактор сразу пригоден для работы без файла: у документа «Без имени»
	// есть одна пустая строка, и в неё можно печатать или вставлять текст.
	newDocument();
}

PagedTextEditor::~PagedTextEditor() {
	BackgroundTaskRegistry::instance().endTask(d->indexTaskId());
	BackgroundTaskRegistry::instance().endTask(d->searchTaskId());
	if (d->currentFormatter) {
		d->currentFormatter->stop();
		d->currentFormatter->disconnect(this);
		d->currentFormatter = nullptr;
		BackgroundTaskRegistry::instance().endTask(d->formatTaskId());
	}
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	if (d->searchCancel) {
		d->searchCancel->store(true);
	}
	if (d->watcher) {
		d->watcher->waitForFinished();
	}
	if (d->searchWatcher) {
		d->searchWatcher->waitForFinished();
	}
}

void PagedTextEditor::openFile(const QString& path, const QString& encoding) {
	BackgroundTaskRegistry::instance().endTask(d->indexTaskId());
	BackgroundTaskRegistry::instance().endTask(d->searchTaskId());
	if (d->currentFormatter) {
		d->currentFormatter->stop();
		d->currentFormatter->disconnect(this);
		d->currentFormatter = nullptr;
		BackgroundTaskRegistry::instance().endTask(d->formatTaskId());
	}
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->clearPages();
	d->chunks.clear();
	d->history.clear();
	d->ready = false;
	d->indexing = false;
	d->error.clear();
	d->lines = 0;
	d->maxLineLength = 0;
	d->fileSize = 0;
	d->lineStarts.clear();
	d->rowOffsetPx = 0;
	d->restoreLine = -1;
	d->restoreCursorLine = -1;
	d->restoreCursorColumn = 0;
	if (d->searchRefreshTimer) {
		d->searchRefreshTimer->stop();
	}
	d->path = path;
	d->encodingName = encoding;
	d->codec = QTextCodec::codecForName(encoding.toLatin1());
	if (!d->codec) {
		d->codec = QTextCodec::codecForName("UTF-8");
	}
	if (d->highlightingManager) {
		setHighlightingPlugin(d->highlightingManager->pluginForFile(path));
	}
	d->resetEditState();
	d->syncSpillState();

	d->searchGeneration++;
	if (d->searchCancel) {
		d->searchCancel->store(true);
	}
	d->searchMatches.clear();
	d->searchTotal = 0;
	d->searchCurrent = -1;
	emit searchCountChanged(0);

	d->updateScrollBars();
	viewport()->update();
	emit lineCountChanged(0);

	const bool multibyte = encoding.startsWith(QStringLiteral("UTF-16"))
		|| encoding.startsWith(QStringLiteral("UTF-32"));
	if (multibyte) {
		const QString message = tr("Прототип поддерживает UTF-8 и однобайтовые "
			"кодировки; UTF-16/UTF-32 пока не поддержан");
		d->error = message;
		viewport()->update();
		emit indexingFinished(false, message);
		return;
	}

	if (path.isEmpty() || !QFile::exists(path)) {
		const QString message = tr("Файл не найден: %1").arg(path);
		d->error = message;
		viewport()->update();
		emit indexingFinished(false, message);
		return;
	}

	auto cancel = std::make_shared<std::atomic_bool>(false);
	d->cancelFlag = cancel;
	d->activeGeneration = d->generation;
	d->indexing = true;
	BackgroundTaskRegistry::instance().beginTask(d->indexTaskId(),
		tr("Индексация файла"));
	viewport()->update();

	d->watcher->setFuture(QtConcurrent::run(
		[path, cancel] {
			return indexPagedFile(path, cancel);
		}));
}

void PagedTextEditor::closeFile() {
	BackgroundTaskRegistry::instance().endTask(d->indexTaskId());
	BackgroundTaskRegistry::instance().endTask(d->searchTaskId());
	if (d->currentFormatter) {
		d->currentFormatter->stop();
		d->currentFormatter->disconnect(this);
		d->currentFormatter = nullptr;
		BackgroundTaskRegistry::instance().endTask(d->formatTaskId());
	}
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->clearPages();
	d->chunks.clear();
	d->history.clear();
	d->ready = false;
	d->indexing = false;
	d->path.clear();
	d->lineStarts.clear();
	d->lines = 0;
	d->maxLineLength = 0;
	d->fileSize = 0;
	if (d->searchRefreshTimer) {
		d->searchRefreshTimer->stop();
	}
	d->resetEditState();
	d->syncSpillState();
	d->updateScrollBars();
	viewport()->update();
	emit lineCountChanged(0);
}

void PagedTextEditor::newDocument() {
	BackgroundTaskRegistry::instance().endTask(d->indexTaskId());
	BackgroundTaskRegistry::instance().endTask(d->searchTaskId());
	if (d->currentFormatter) {
		d->currentFormatter->stop();
		d->currentFormatter->disconnect(this);
		d->currentFormatter = nullptr;
		BackgroundTaskRegistry::instance().endTask(d->formatTaskId());
	}
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->chunks.clear();
	d->history.clear();
	d->searchGeneration++;
	if (d->searchRefreshTimer) {
		d->searchRefreshTimer->stop();
	}
	if (d->searchCancel) {
		d->searchCancel->store(true);
	}
	d->searching = false;
	d->searchMatches.clear();
	d->searchTotal = 0;
	d->searchCurrent = -1;
	d->buffer.resetEmpty(d->encodingName.toLatin1());
	d->path.clear();
	d->lineStarts = { 0 };
	d->fileSize = 0;
	d->lines = d->buffer.lineCount();
	d->maxLineLength = 0;
	d->indexing = false;
	d->ready = true;
	d->error.clear();
	d->cursorLine = 0;
	d->cursorColumn = 0;
	d->anchorLine = 0;
	d->anchorColumn = 0;
	d->rowOffsetPx = 0;
	d->restoreLine = -1;
	d->restoreCursorLine = -1;
	d->resetEditState();
	d->updateScrollBars();
	viewport()->update();
	emit lineCountChanged(d->lines);
	emit searchCountChanged(0);
	emit searchCurrentChanged(-1, 0);
	emit cursorPositionChanged(d->cursorLine, d->cursorColumn);
	d->syncSpillState();
}

QString PagedTextEditor::filePath() const {
	return d->path;
}

bool PagedTextEditor::isIndexing() const {
	return d->indexing;
}

int PagedTextEditor::lineCount() const {
	return d->lines;
}

int PagedTextEditor::maxLineLength() const {
	return d->maxLineLength;
}

int PagedTextEditor::firstVisibleLine() const {
	return verticalScrollBar()->value();
}

QString PagedTextEditor::lineText(int line) const {
	return d->lineAt(line);
}

int PagedTextEditor::cursorLine() const {
	return d->cursorLine;
}

int PagedTextEditor::cursorColumn() const {
	return d->cursorColumn;
}

bool PagedTextEditor::hasSelection() const {
	return d->hasSelection();
}

QString PagedTextEditor::displayedLineText(int line) const {
	return d->visibleText(line);
}

int PagedTextEditor::cursorVisualRow() const {
	if (!d->ready || d->cursorLine < 0 || d->cursorLine >= d->lines) {
		return 0;
	}

	const QString text = d->visibleText(d->cursorLine);
	const int column = qMin(d->cursorColumn, text.size());
	const int layoutWidth = qMax(1, viewport()->width() - d->gutterWidth() - 8);
	TextLayout layout(text, font(), d->wrapLines, layoutWidth, TextLayout::kDefaultTabWidth);
	return layout.visualRowForColumn(column);
}

bool PagedTextEditor::isCursorVisible() const {
	if (!d->ready) {
		return false;
	}

	int cursorY = 0;
	int cursorHeight = 0;
	if (!d->cursorRowGeometry(d->cursorLine, cursorY, cursorHeight)) {
		return false;
	}

	int visibleY = -d->rowOffsetPx + cursorY;
	const int layoutWidth = qMax(1, viewport()->width() - d->gutterWidth() - 8);
	for (int i = verticalScrollBar()->value(); i < d->cursorLine; i++) {
		TextLayout layout(d->visibleText(i), font(), d->wrapLines, layoutWidth, TextLayout::kDefaultTabWidth);
		for (int visual = 0; visual < layout.lineCount(); visual++) {
			visibleY += qMax(1, qCeil(layout.lineAt(visual).height()));
		}
	}

	return visibleY >= 0
		&& visibleY + cursorHeight <= viewport()->height();
}

void PagedTextEditor::setCursorPosition(int line, int column, bool keepAnchor) {
	if (!d->ready) {
		return;
	}

	line = qBound(0, line, qMax(0, d->lines - 1));
	const int lineLength = d->lineAt(line).size();
	column = qBound(0, column, lineLength);

	d->cursorLine = line;
	d->cursorColumn = column;
	d->searchSelectionActive = false;
	if (!keepAnchor) {
		d->anchorLine = line;
		d->anchorColumn = column;
	}

	d->ensureCursorVisible();
	d->updateCurrentForCursor();
	viewport()->update();
	emit cursorPositionChanged(line, column);
}

void PagedTextEditor::selectAll() {
	if (!d->ready) {
		return;
	}

	d->anchorLine = 0;
	d->anchorColumn = 0;
	d->cursorLine = qMax(0, d->lines - 1);
	d->cursorColumn = d->lineAt(d->cursorLine).size();
	d->searchSelectionActive = false;
	d->ensureCursorVisible();
	d->updateCurrentForCursor();
	viewport()->update();
	emit cursorPositionChanged(d->cursorLine, d->cursorColumn);
}

QString PagedTextEditor::selectedText() const {
	if (!d->hasSelection() || !d->ready) {
		return QString();
	}

	int startLine = 0;
	int startColumn = 0;
	int endLine = 0;
	int endColumn = 0;
	d->normalizedSelection(startLine, startColumn, endLine, endColumn);

	QString result;
	for (int line = startLine; line <= endLine; line++) {
		const QString text = d->lineAt(line);
		const int from = line == startLine ? startColumn : 0;
		const int to = line == endLine ? endColumn : text.size();
		result += text.mid(from, qMax(0, to - from));
		if (line < endLine) {
			result += QLatin1Char('\n');
		}
	}
	return result;
}

QString PagedTextEditor::encoding() const {
	return d->encodingName;
}

bool PagedTextEditor::setEncoding(const QString& encoding) {
	if (encoding.isEmpty() || encoding == d->encodingName) {
		return false;
	}

	// Перечитать файл можно только без несохранённых правок: иначе они
	// пропали бы вместе со страницами в памяти.
	if (d->buffer.hasChanges()) {
		return false;
	}

	if (d->path.isEmpty()) {
		d->encodingName = encoding;
		d->buffer.resetEmpty(encoding.toLatin1());
		return true;
	}

	const int oldLine = verticalScrollBar()->value();
	const int oldCursorLine = d->cursorLine;
	const int oldCursorColumn = d->cursorColumn;
	openFile(d->path, encoding);
	d->restoreLine = oldLine;
	d->restoreCursorLine = oldCursorLine;
	d->restoreCursorColumn = oldCursorColumn;
	return true;
}

bool PagedTextEditor::isReadOnly() const {
	return d->readOnly;
}

void PagedTextEditor::setReadOnly(bool readOnly) {
	d->readOnly = readOnly;
}

bool PagedTextEditor::isModified() const {
	return d->buffer.hasChanges();
}

int PagedTextEditor::modifiedPageCount() const {
	return d->buffer.dirtyPageCount();
}

void PagedTextEditor::setMemoryBudget(qint64 bytes) {
	d->buffer.setMemoryBudget(bytes);
	d->syncSpillState();
}

qint64 PagedTextEditor::memoryBudget() const {
	return d->buffer.memoryBudget();
}

qint64 PagedTextEditor::bufferedBytes() const {
	return d->buffer.bufferedBytes();
}

int PagedTextEditor::spilledPageCount() const {
	return d->buffer.spilledPageCount();
}

QString PagedTextEditor::spillFilePath() const {
	return d->buffer.spillFilePath();
}

bool PagedTextEditor::save() {
	if (d->path.isEmpty()) {
		emit saved(QString(), false, tr("Документ не имеет пути для сохранения"));
		return false;
	}
	return saveAs(d->path);
}

bool PagedTextEditor::saveAs(const QString& path) {
	if (path.isEmpty()) {
		emit saved(path, false, tr("Не задан путь для сохранения"));
		return false;
	}
	if (!d->ready || d->indexing) {
		emit saved(path, false, tr("Файл ещё не прочитан"));
		return false;
	}

	QString error;
	if (!d->buffer.saveTo(path, &error)) {
		emit saved(path, false, error);
		return false;
	}

	// После записи файл переиндексируется: индекс строк и страницы строятся
	// заново, а курсор и прокрутка сохраняются.
	const int oldLine = verticalScrollBar()->value();
	const int oldCursorLine = d->cursorLine;
	const int oldCursorColumn = d->cursorColumn;
	if (d->searchRefreshTimer) {
		d->searchRefreshTimer->stop();
	}
	openFile(path, d->encodingName);
	d->restoreLine = oldLine;
	d->restoreCursorLine = oldCursorLine;
	d->restoreCursorColumn = oldCursorColumn;
	emit saved(path, true, QString());
	return true;
}

bool PagedTextEditor::canUndo() const {
	return d->history.canUndo();
}

bool PagedTextEditor::canRedo() const {
	return d->history.canRedo();
}

bool PagedTextEditor::undo() {
	if (!d->canEdit()) {
		return false;
	}

	PagedEditRecord record;
	if (!d->history.takeUndo(record)) {
		return false;
	}

	for (const auto& entry : record.before) {
		d->buffer.restorePage(entry.first, entry.second);
	}
	// Восстановленные страницы снова занимают память: держим её в пределе.
	d->buffer.enforceMemoryBudget();
	d->applyHistoryState(record.cursorBefore);
	return true;
}

bool PagedTextEditor::redo() {
	if (!d->canEdit()) {
		return false;
	}

	PagedEditRecord record;
	if (!d->history.takeRedo(record)) {
		return false;
	}

	for (const auto& entry : record.after) {
		d->buffer.restorePage(entry.first, entry.second);
	}
	d->buffer.enforceMemoryBudget();
	d->applyHistoryState(record.cursorAfter);
	return true;
}

bool PagedTextEditor::insertText(const QString& text) {
	if (text.isEmpty() || !d->canEdit()) {
		return false;
	}

	QString normalized = text;
	normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
	normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));

	int startLine = d->cursorLine;
	int startColumn = d->cursorColumn;
	int endLine = d->cursorLine;
	int endColumn = d->cursorColumn;
	if (d->hasSelection()) {
		d->normalizedSelection(startLine, startColumn, endLine, endColumn);
	}

	// Один печатный символ склеивается с предыдущим набором в один шаг отмены.
	const int mergeKind = normalized.size() == 1 ? kMergeInsert : 0;
	return d->replaceRange(startLine, startColumn, endLine, endColumn,
		normalized, mergeKind);
}

bool PagedTextEditor::deleteSelection() {
	if (!d->canEdit()) {
		return false;
	}

	int startLine = d->cursorLine;
	int startColumn = d->cursorColumn;
	int endLine = d->cursorLine;
	int endColumn = d->cursorColumn;
	if (d->hasSelection()) {
		d->normalizedSelection(startLine, startColumn, endLine, endColumn);
	}
	return d->replaceRange(startLine, startColumn, endLine, endColumn,
		QString(), 0);
}

void PagedTextEditor::copy() {
	if (!d->ready) {
		return;
	}

	QString text = selectedText();
	if (text.isEmpty()) {
		text = d->lineAt(d->cursorLine);
	}
	QGuiApplication::clipboard()->setText(text);
}

void PagedTextEditor::cut() {
	if (!d->canEdit() || !d->hasSelection()) {
		return;
	}
	copy();
	deleteSelection();
}

void PagedTextEditor::paste() {
	if (!d->canEdit()) {
		return;
	}

	const QString text = QGuiApplication::clipboard()->text();
	if (!text.isEmpty()) {
		insertText(text);
	}
}

void PagedTextEditor::setHighlightingManager(HighlightingPluginManager* manager) {
	d->highlightingManager = manager;
	if (manager && !d->path.isEmpty()) {
		setHighlightingPlugin(manager->pluginForFile(d->path));
	}
}

void PagedTextEditor::setFormatterManager(FormatterPluginManager* manager) {
	d->formatterManager = manager;
}

bool PagedTextEditor::setHighlightingPlugin(HighlightingPlugin* plugin) {
	if (d->highlighter.plugin() == plugin) {
		return false;
	}

	d->highlighter.setPlugin(plugin);
	const bool enabled = plugin != nullptr;
	if (d->highlightingEnabled != enabled) {
		d->highlightingEnabled = enabled;
		emit highlightingEnabledChanged(enabled);
	}
	viewport()->update();
	emit highlightingChanged();
	return true;
}

HighlightingPlugin* PagedTextEditor::highlightingPlugin() const {
	return d->highlighter.plugin();
}

bool PagedTextEditor::isHighlightingEnabled() const {
	return d->highlightingEnabled && d->highlighter.plugin() != nullptr;
}

void PagedTextEditor::setHighlightingEnabled(bool enabled) {
	if (d->highlightingEnabled == enabled) {
		return;
	}
	d->highlightingEnabled = enabled;
	viewport()->update();
	emit highlightingEnabledChanged(enabled);
}

bool PagedTextEditor::canFormat() const {
	if (!d->ready || d->currentFormatter || !d->formatterManager) {
		return false;
	}
	const auto* plugin = d->highlighter.plugin();
	return plugin != nullptr
		&& d->formatterManager->formatter(plugin->languageInfo()) != nullptr;
}

bool PagedTextEditor::isFormatting() const {
	return d->currentFormatter != nullptr;
}

bool PagedTextEditor::formatDocument() {
	if (!canFormat()) {
		return false;
	}

	// Форматирование читает и перезаписывает файл целиком: правки из памяти
	// при этом потерялись бы, поэтому сначала нужно сохранить документ.
	if (isModified()) {
		emit formattingFailed(tr("Есть несохранённые изменения: сохраните файл "
			"перед форматированием"));
		return false;
	}

	QFile file(d->path);
	if (!file.open(QIODevice::ReadOnly)) {
		emit formattingFailed(tr("Не удалось открыть файл: %1").arg(d->path));
		return false;
	}

	QTextCodec* codec = d->codec ? d->codec : QTextCodec::codecForName("UTF-8");
	const QByteArray bytes = file.readAll();
	file.close();
	QString source = codec ? codec->toUnicode(bytes) : QString::fromUtf8(bytes);
	if (source.startsWith(QChar(0xFEFF))) {
		source.remove(0, 1);
	}

	auto* formatter = d->formatterManager->formatter(
		d->highlighter.plugin()->languageInfo());
	if (!formatter) {
		return false;
	}

	d->currentFormatter = formatter;
	d->formatSource = source;
	BackgroundTaskRegistry::instance().beginTask(d->formatTaskId(),
		tr("Форматирование файла"));

	connect(formatter, &FormatterPlugin::formattingFinished, this,
		[this, formatter, codec](const QString& formattedText) {
			if (d->currentFormatter != formatter) {
				return;
			}
			d->currentFormatter = nullptr;
			formatter->disconnect(this);
			BackgroundTaskRegistry::instance().endTask(d->formatTaskId());

			const bool changed = formattedText != d->formatSource;
			if (changed) {
				QString output = formattedText;
				if (d->formatSource.contains(QStringLiteral("\r\n"))) {
					output.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
					output.replace(QStringLiteral("\n"), QStringLiteral("\r\n"));
				}

				QSaveFile save(d->path);
				if (!save.open(QIODevice::WriteOnly)
					|| save.write(codec ? codec->fromUnicode(output)
						: output.toUtf8()) < 0
					|| !save.commit()) {
					emit formattingFailed(tr("Не удалось записать файл: %1").arg(d->path));
					return;
				}

				const int oldLine = verticalScrollBar()->value();
				const int oldCursorLine = d->cursorLine;
				const int oldCursorColumn = d->cursorColumn;
				openFile(d->path, d->encodingName);
				d->restoreLine = oldLine;
				d->restoreCursorLine = oldCursorLine;
				d->restoreCursorColumn = oldCursorColumn;
			}

			emit formattingFinished(changed);
		});

	connect(formatter, &FormatterPlugin::formattingFailed, this,
		[this, formatter](const QString& errorMessage) {
			if (d->currentFormatter != formatter) {
				return;
			}
			d->currentFormatter = nullptr;
			formatter->disconnect(this);
			BackgroundTaskRegistry::instance().endTask(d->formatTaskId());
			emit formattingFailed(errorMessage);
		});

	emit formattingStarted();
	formatter->formatAsync(source);
	return true;
}

qint64 PagedTextEditor::fileSize() const {
	return d->fileSize;
}

int PagedTextEditor::fontSize() const {
	const int pointSize = font().pointSize();
	return pointSize > 0 ? pointSize : 10;
}

void PagedTextEditor::setFontSize(int size) {
	if (size <= 0 || size == font().pointSize()) {
		return;
	}

	const int first = verticalScrollBar()->value();
	QFont current = font();
	current.setPointSize(size);
	setFont(current);
	d->rowOffsetPx = 0;
	d->updateScrollBars();
	verticalScrollBar()->setValue(qMin(first, verticalScrollBar()->maximum()));
	viewport()->update();
	emit fontSizeChanged(size);
}

bool PagedTextEditor::wordWrap() const {
	return d->wrapLines;
}

void PagedTextEditor::setWordWrap(bool enabled) {
	if (d->wrapLines == enabled) {
		return;
	}

	const int first = verticalScrollBar()->value();
	d->wrapLines = enabled;
	d->rowOffsetPx = 0;
	setHorizontalScrollBarPolicy(enabled ? Qt::ScrollBarAlwaysOff : Qt::ScrollBarAsNeeded);
	d->updateScrollBars();
	verticalScrollBar()->setValue(qMin(first, verticalScrollBar()->maximum()));
	viewport()->update();
	emit wordWrapChanged(enabled);
}

bool PagedTextEditor::lineNumbersVisible() const {
	return d->showLineNumbers;
}

void PagedTextEditor::setLineNumbersVisible(bool visible) {
	if (d->showLineNumbers == visible) {
		return;
	}

	const int first = verticalScrollBar()->value();
	d->showLineNumbers = visible;
	d->updateScrollBars();
	verticalScrollBar()->setValue(qMin(first, verticalScrollBar()->maximum()));
	viewport()->update();
	emit lineNumbersVisibleChanged(visible);
}

void PagedTextEditor::setSearchQuery(const QString& query) {
	d->startSearch(query, true);
}

void PagedTextEditor::setSearchCaseSensitive(bool enabled) {
	if (d->searchCaseSensitive == enabled) {
		return;
	}

	d->searchCaseSensitive = enabled;
	if (!d->searchText.isEmpty()) {
		d->startSearch(d->searchText, true);
	}
}

bool PagedTextEditor::searchCaseSensitive() const {
	return d->searchCaseSensitive;
}

QString PagedTextEditor::searchQuery() const {
	return d->searchText;
}

int PagedTextEditor::searchMatchCount() const {
	return d->searchTotal;
}

int PagedTextEditor::currentSearchIndex() const {
	return d->searchCurrent;
}

void PagedTextEditor::searchNext() {
	if (d->searchMatches.isEmpty()) {
		return;
	}

	int index = -1;
	const auto isInsideCurrent = [this] {
		if (!d->searchSelectionActive
			|| d->searchCurrent < 0 || d->searchCurrent >= d->searchMatches.size()) {
			return false;
		}
		const auto& match = d->searchMatches.at(d->searchCurrent);
		return d->cursorLine == match.line
			&& d->cursorColumn >= match.start
			&& d->cursorColumn < match.start + match.length;
	};

	if (isInsideCurrent()) {
		index = (d->searchCurrent + 1) % d->searchMatches.size();
	}
	else {
		for (int i = 0; i < d->searchMatches.size(); i++) {
			const auto& match = d->searchMatches.at(i);
			if (match.line > d->cursorLine
				|| (match.line == d->cursorLine
					&& match.start >= d->cursorColumn)) {
				index = i;
				break;
			}
		}
		if (index < 0) {
			index = 0;
		}
	}

	d->selectMatch(index);
}

void PagedTextEditor::searchPrevious() {
	if (d->searchMatches.isEmpty()) {
		return;
	}

	int index = -1;
	const auto isInsideCurrent = [this] {
		if (!d->searchSelectionActive
			|| d->searchCurrent < 0 || d->searchCurrent >= d->searchMatches.size()) {
			return false;
		}
		const auto& match = d->searchMatches.at(d->searchCurrent);
		return d->cursorLine == match.line
			&& d->cursorColumn >= match.start
			&& d->cursorColumn < match.start + match.length;
	};

	if (isInsideCurrent()) {
		index = (d->searchCurrent - 1 + d->searchMatches.size())
			% d->searchMatches.size();
	}
	else {
		for (int i = 0; i < d->searchMatches.size(); i++) {
			const auto& match = d->searchMatches.at(i);
			if (match.line < d->cursorLine
				|| (match.line == d->cursorLine
					&& match.start < d->cursorColumn)) {
				index = i;
			}
			else {
				break;
			}
		}
		if (index < 0) {
			index = d->searchMatches.size() - 1;
		}
	}

	d->selectMatch(index);
}

void PagedTextEditor::paintEvent(QPaintEvent* event) {
	Q_UNUSED(event);
	QPainter painter(viewport());
	if (d->indexing) {
		painter.fillRect(viewport()->rect(), palette().base());
		painter.setPen(palette().text().color());
		painter.drawText(viewport()->rect(), Qt::AlignCenter,
			tr("Индексация файла…"));
		return;
	}

	if (!d->ready) {
		painter.fillRect(viewport()->rect(), palette().base());
		painter.setPen(palette().placeholderText().color());
		painter.drawText(viewport()->rect().adjusted(8, 8, -8, -8),
			Qt::AlignLeft | Qt::AlignTop, d->error);
		return;
	}

	PagedTextPaintModel model;
	model.viewport = viewport()->rect();
	model.palette = palette();
	model.font = font();
	model.firstLine = verticalScrollBar()->value();
	model.lineCount = d->lines;
	model.horizontalScroll = d->wrapLines ? 0 : horizontalScrollBar()->value();
	model.rowOffsetPx = d->rowOffsetPx;
	model.gutterWidth = d->gutterWidth();
	model.wrapped = d->wrapLines;
	model.lineNumbers = d->showLineNumbers;
	model.hasFocus = hasFocus();
	model.cursorLine = d->cursorLine;
	model.cursorColumn = d->cursorColumn;
	model.hasSelection = d->hasSelection();
	if (model.hasSelection) {
		d->normalizedSelection(model.selectionStartLine, model.selectionStartColumn,
			model.selectionEndLine, model.selectionEndColumn);
	}
	model.token = d->selectedToken();
	model.searchMatches = &d->searchMatches;
	model.currentSearch = d->searchCurrent;
	model.rawLine = [this](int line) { return d->lineAt(line); };
	model.displayLine = [this](int line) { return d->visibleText(line); };
	model.highlightSpans = [this](int line) {
		if (!d->highlightingEnabled) {
			return QVector<PagedTextSpan>();
		}
		return d->highlighter.spansForLine(d->visibleText(line));
	};

	PagedTextRenderer::paint(painter, model);
}

void PagedTextEditor::resizeEvent(QResizeEvent* event) {
	QAbstractScrollArea::resizeEvent(event);
	d->updateScrollBars();
}

void PagedTextEditor::scrollContentsBy(int dx, int dy) {
	Q_UNUSED(dx);
	Q_UNUSED(dy);
	viewport()->update();
}

void PagedTextEditor::wheelEvent(QWheelEvent* event) {
	if (event->modifiers() & Qt::ControlModifier) {
		const int delta = event->angleDelta().y();
		if (delta != 0) {
			setFontSize(qMax(1, fontSize() + (delta > 0 ? 1 : -1)));
		}
		event->accept();
		return;
	}

	QAbstractScrollArea::wheelEvent(event);
}

void PagedTextEditor::mousePressEvent(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton || !d->ready) {
		QAbstractScrollArea::mousePressEvent(event);
		return;
	}

	setFocus(Qt::MouseFocusReason);
	const int gutter = d->gutterWidth();
	const int clickX = int(event->position().x());
	const int clickY = int(event->position().y());

	// Клик по номеру строки выделяет строку целиком.
	if (clickX < gutter) {
		const int line = d->logicalLineAtY(clickY);
		d->selecting = false;
		d->anchorLine = line;
		d->anchorColumn = 0;
		d->cursorLine = line;
		d->cursorColumn = d->lineAt(line).size();
		d->searchSelectionActive = false;
		d->ensureCursorVisible();
		d->updateCurrentForCursor();
		viewport()->update();
		emit cursorPositionChanged(line, 0);
		event->accept();
		return;
	}

	int line = 0;
	int column = 0;
	if (!d->positionAt(clickX, clickY, line, column)) {
		event->accept();
		return;
	}

	if (event->modifiers() & Qt::ShiftModifier) {
		setCursorPosition(line, column, true);
	}
	else {
		d->selecting = true;
		setCursorPosition(line, column, false);
	}
	event->accept();
}

void PagedTextEditor::mouseMoveEvent(QMouseEvent* event) {
	if (!d->selecting || !d->ready) {
		QAbstractScrollArea::mouseMoveEvent(event);
		return;
	}

	int line = 0;
	int column = 0;
	if (!d->positionAt(int(event->position().x()), int(event->position().y()),
		line, column)) {
		QAbstractScrollArea::mouseMoveEvent(event);
		return;
	}

	if (event->position().y() < 0 && verticalScrollBar()->value() > 0) {
		verticalScrollBar()->setValue(verticalScrollBar()->value() - 1);
	}
	else if (event->position().y() > viewport()->height()) {
		verticalScrollBar()->setValue(verticalScrollBar()->value() + 1);
	}

	setCursorPosition(line, column, true);
	event->accept();
}

void PagedTextEditor::mouseReleaseEvent(QMouseEvent* event) {
	d->selecting = false;
	QAbstractScrollArea::mouseReleaseEvent(event);
}

void PagedTextEditor::mouseDoubleClickEvent(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton || !d->ready) {
		QAbstractScrollArea::mouseDoubleClickEvent(event);
		return;
	}

	const auto isWord = [](QChar character) {
		return character.isLetterOrNumber() || character == QLatin1Char('_');
	};

	int line = 0;
	int column = 0;
	if (!d->positionAt(int(event->position().x()), int(event->position().y()),
		line, column)) {
		event->accept();
		return;
	}
	QString text = d->lineAt(line);
	int start = qMin(column, text.size());
	int end = start;

	while (start > 0 && isWord(text.at(start - 1))) {
		start--;
	}
	while (end < text.size() && isWord(text.at(end))) {
		end++;
	}

	if (start != end) {
		d->anchorLine = line;
		d->anchorColumn = end;
		d->cursorLine = line;
		d->cursorColumn = start;
		d->searchSelectionActive = false;
		d->updateCurrentForCursor();
		emit cursorPositionChanged(line, start);
		viewport()->update();
	}
	event->accept();
}

void PagedTextEditor::keyPressEvent(QKeyEvent* event) {
	if (!d->ready) {
		QAbstractScrollArea::keyPressEvent(event);
		return;
	}

	const bool shift = (event->modifiers() & Qt::ShiftModifier) != 0;
	const bool control = (event->modifiers() & Qt::ControlModifier) != 0;
	const int lineLength = d->lineAt(d->cursorLine).size();

	if (event->matches(QKeySequence::Undo)) {
		undo();
		event->accept();
		return;
	}

	if (event->matches(QKeySequence::Redo)) {
		redo();
		event->accept();
		return;
	}

	if (event->matches(QKeySequence::Cut)) {
		cut();
		event->accept();
		return;
	}

	if (event->matches(QKeySequence::Copy)) {
		copy();
		event->accept();
		return;
	}

	if (event->matches(QKeySequence::Paste)) {
		paste();
		event->accept();
		return;
	}

	if (event->matches(QKeySequence::SelectAll)) {
		selectAll();
		event->accept();
		return;
	}

	switch (event->key()) {
	case Qt::Key_Backspace:
		d->backspace();
		event->accept();
		return;
	case Qt::Key_Delete:
		d->deleteForward();
		event->accept();
		return;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		insertText(QStringLiteral("\n"));
		event->accept();
		return;
	case Qt::Key_Tab:
		insertText(QStringLiteral("\t"));
		event->accept();
		return;
	default:
		break;
	}

	// Обычный ввод текста: раскладка уже преобразовала нажатие в символ.
	const QString typed = event->text();
	if (!typed.isEmpty() && typed.at(0).isPrint()
		&& !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier))) {
		insertText(typed);
		event->accept();
		return;
	}

	const int pageStep = qMax(1,
		viewport()->height() / qMax(1, d->lineHeight()) - 1);

	switch (event->key()) {
	case Qt::Key_Up:
		d->moveCursorVertically(-1, shift);
		return;
	case Qt::Key_Down:
		d->moveCursorVertically(1, shift);
		return;
	case Qt::Key_PageUp:
		setCursorPosition(d->cursorLine - pageStep, d->cursorColumn, shift);
		return;
	case Qt::Key_PageDown:
		setCursorPosition(d->cursorLine + pageStep, d->cursorColumn, shift);
		return;
	case Qt::Key_Left:
		if (d->cursorColumn > 0) {
			setCursorPosition(d->cursorLine, d->cursorColumn - 1, shift);
		}
		else if (d->cursorLine > 0) {
			const int previous = d->lineAt(d->cursorLine - 1).size();
			setCursorPosition(d->cursorLine - 1, previous, shift);
		}
		return;
	case Qt::Key_Right:
		if (d->cursorColumn < lineLength) {
			setCursorPosition(d->cursorLine, d->cursorColumn + 1, shift);
		}
		else if (d->cursorLine + 1 < d->lines) {
			setCursorPosition(d->cursorLine + 1, 0, shift);
		}
		return;
	case Qt::Key_Home:
		setCursorPosition(control ? 0 : d->cursorLine, 0, shift);
		return;
	case Qt::Key_End:
		setCursorPosition(control ? qMax(0, d->lines - 1) : d->cursorLine,
			control ? d->lineAt(qMax(0, d->lines - 1)).size() : lineLength, shift);
		return;
	default:
		break;
	}

	QAbstractScrollArea::keyPressEvent(event);
}

void PagedTextEditor::inputMethodEvent(QInputMethodEvent* event) {
	if (!d->canEdit() || event->commitString().isEmpty()) {
		QAbstractScrollArea::inputMethodEvent(event);
		return;
	}

	insertText(event->commitString());
	event->accept();
}

void PagedTextEditor::contextMenuEvent(QContextMenuEvent* event) {
	QMenu menu(this);
	QAction* undoAction = menu.addAction(tr("Отменить"));
	undoAction->setEnabled(canUndo() && !isReadOnly());
	QAction* redoAction = menu.addAction(tr("Повторить"));
	redoAction->setEnabled(canRedo() && !isReadOnly());
	menu.addSeparator();
	QAction* cutAction = menu.addAction(tr("Вырезать"));
	cutAction->setEnabled(!isReadOnly() && d->hasSelection());
	QAction* copyAction = menu.addAction(tr("Копировать"));
	copyAction->setEnabled(d->ready);
	QAction* pasteAction = menu.addAction(tr("Вставить"));
	pasteAction->setEnabled(!isReadOnly() && !QGuiApplication::clipboard()->text().isEmpty());
	menu.addSeparator();
	QAction* selectAllAction = menu.addAction(tr("Выделить всё"));
	selectAllAction->setEnabled(d->ready);

	QAction* chosen = menu.exec(event->globalPos());
	if (chosen == undoAction) {
		undo();
	}
	else if (chosen == redoAction) {
		redo();
	}
	else if (chosen == cutAction) {
		cut();
	}
	else if (chosen == copyAction) {
		copy();
	}
	else if (chosen == pasteAction) {
		paste();
	}
	else if (chosen == selectAllAction) {
		selectAll();
	}
	event->accept();
}
