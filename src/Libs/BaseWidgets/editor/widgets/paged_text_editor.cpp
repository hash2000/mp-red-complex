#include "Libs/BaseWidgets/editor/widgets/paged_text_editor.h"
#include "Libs/Base/background_task_registry.h"
#include "Libs/BaseWidgets/editor/layout/text_layout.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_file_indexer.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_line_cache.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_line_chunks.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_cursor.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_search.h"
#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_renderer.h"

#include <QClipboard>
#include <QFile>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QHash>
#include <QKeyEvent>
#include <QKeySequence>
#include <QList>
#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextCodec>
#include <QTextLine>
#include <QWheelEvent>
#include <QtConcurrent/QtConcurrentRun>
#include <QtMath>

#include <atomic>
#include <algorithm>
#include <limits>
#include <memory>

namespace {

/// Дополнительный отступ справа, чтобы последний символ не упирался в край.
constexpr int kHorizontalMarginPx = 24;

} // namespace

struct PagedTextEditor::Private {
	PagedTextEditor* q = nullptr;

	QString path;
	QString encodingName;
	QTextCodec* codec = nullptr;

	bool indexing = false;
	bool ready = false;
	bool wrapLines = false;
	bool showLineNumbers = true;
	QString error;

	int lines = 0;
	int maxLineLength = 0;
	qint64 fileSize = 0;
	QVector<qint64> lineStarts;

	PagedLineCache cache;
	PagedLineChunks chunks;

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

	QString searchText;
	QVector<PagedSearchMatch> searchMatches;
	int searchTotal = 0;
	int searchCurrent = -1;
	bool searching = false;
	bool searchSelectionActive = false;
	bool searchCaseSensitive = false;
	int rowOffsetPx = 0;
	bool loadingMoreHorizontally = false;
	QFutureWatcher<PagedSearchOutcome>* searchWatcher = nullptr;
	std::shared_ptr<std::atomic_bool> searchCancel;
	quint64 searchGeneration = 0;
	quint64 searchActiveGeneration = 0;

	void clearPages();
	QString indexTaskId() const;
	QString searchTaskId() const;
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
};

void PagedTextEditor::Private::clearPages() {
	cache.clear();
}

QString PagedTextEditor::Private::indexTaskId() const {
	return QStringLiteral("paged-index:%1").arg(quintptr(q), 0, 16);
}

QString PagedTextEditor::Private::searchTaskId() const {
	return QStringLiteral("paged-search:%1").arg(quintptr(q), 0, 16);
}

QString PagedTextEditor::Private::lineAt(int line) const {
	if (!ready || line < 0 || line >= lines) {
		return QString();
	}
	return cache.lineAt(line);
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
		d->cache.reset(d->path, d->encodingName.toLatin1(), d->lineStarts, d->fileSize);
		d->ready = true;
		d->error.clear();
		d->chunks.clear();
		d->updateScrollBars();
		verticalScrollBar()->setValue(d->restoreLine >= 0
			? qMin(d->restoreLine, qMax(0, d->lines - 1)) : 0);
		d->restoreLine = -1;
		d->cursorLine = 0;
		d->cursorColumn = 0;
		d->anchorLine = 0;
		d->anchorColumn = 0;
		viewport()->update();
		emit lineCountChanged(d->lines);
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

		viewport()->update();
		emit searchCountChanged(d->searchTotal);
	});

	connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] {
		d->rowOffsetPx = 0;
	});
	connect(horizontalScrollBar(), &QScrollBar::valueChanged, this, [this] {
		d->maybeLoadMoreHorizontally();
	});

	d->updateScrollBars();
}

PagedTextEditor::~PagedTextEditor() {
	BackgroundTaskRegistry::instance().endTask(d->indexTaskId());
	BackgroundTaskRegistry::instance().endTask(d->searchTaskId());
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
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->clearPages();
	d->chunks.clear();
	d->ready = false;
	d->indexing = false;
	d->error.clear();
	d->lines = 0;
	d->maxLineLength = 0;
	d->fileSize = 0;
	d->lineStarts.clear();
	d->rowOffsetPx = 0;
	d->path = path;
	d->encodingName = encoding;
	d->codec = QTextCodec::codecForName(encoding.toLatin1());
	if (!d->codec) {
		d->codec = QTextCodec::codecForName("UTF-8");
	}

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
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->clearPages();
	d->chunks.clear();
	d->ready = false;
	d->indexing = false;
	d->path.clear();
	d->lineStarts.clear();
	d->lines = 0;
	d->maxLineLength = 0;
	d->fileSize = 0;
	d->updateScrollBars();
	viewport()->update();
	emit lineCountChanged(0);
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

void PagedTextEditor::setEncoding(const QString& encoding) {
	if (encoding.isEmpty() || encoding == d->encodingName || d->path.isEmpty()) {
		return;
	}

	const int oldLine = verticalScrollBar()->value();
	openFile(d->path, encoding);
	d->restoreLine = oldLine;
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
}void PagedTextEditor::setWordWrap(bool enabled) {
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
	d->searchText = query;
	d->searchGeneration++;
	if (d->searchCancel) {
		d->searchCancel->store(true);
	}
	d->searching = false;
	d->searchMatches.clear();
	d->searchTotal = 0;
	d->searchCurrent = -1;
	emit searchCurrentChanged(-1, 0);
	viewport()->update();

	if (!d->ready || query.isEmpty()) {
		BackgroundTaskRegistry::instance().endTask(d->searchTaskId());
		emit searchCountChanged(0);
		return;
	}

	emit searchCountChanged(-1);
	BackgroundTaskRegistry::instance().beginTask(d->searchTaskId(),
		tr("Поиск: %1").arg(query));
	auto cancel = std::make_shared<std::atomic_bool>(false);
	d->searchCancel = cancel;
	d->searchActiveGeneration = d->searchGeneration;
	d->searching = true;

	d->searchWatcher->setFuture(QtConcurrent::run(
		[path = d->path, encoding = d->encodingName.toLatin1(),
			starts = d->lineStarts, size = d->fileSize, query,
			caseSensitivity = d->searchCaseSensitive ? Qt::CaseSensitive
				: Qt::CaseInsensitive, cancel,
			this, generation = d->searchGeneration] {
			return runPagedLineSearch(path, encoding, starts, size, query,
				caseSensitivity, cancel,
				[this, generation](int total) {
					QMetaObject::invokeMethod(this,
						[this, generation, total] {
							if (d && generation == d->searchGeneration) {
								emit searchProgressChanged(total);
							}
						}, Qt::QueuedConnection);
				});
		}));
}

void PagedTextEditor::setSearchCaseSensitive(bool enabled) {
	if (d->searchCaseSensitive == enabled) {
		return;
	}

	d->searchCaseSensitive = enabled;
	if (!d->searchText.isEmpty()) {
		setSearchQuery(d->searchText);
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

	if (event->matches(QKeySequence::Copy)) {
		QString text = selectedText();
		if (text.isEmpty()) {
			text = d->lineAt(d->cursorLine);
		}
		QGuiApplication::clipboard()->setText(text);
		event->accept();
		return;
	}

	if (event->matches(QKeySequence::SelectAll)) {
		selectAll();
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
