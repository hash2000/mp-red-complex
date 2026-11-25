#include "Libs/BaseWidgets/editor/widgets/paged_text_editor.h"

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
#include <QTextLayout>
#include <QTextLine>
#include <QTextOption>
#include <QTextLayout>
#include <QWheelEvent>
#include <QtConcurrent/QtConcurrentRun>
#include <QtMath>

#include <atomic>
#include <algorithm>
#include <limits>
#include <memory>

namespace {

/// Сколько строк держим в одной странице.
constexpr int kPageLines = 2000;
/// Ширина табуляции в символах (как в редакторе кода: TextEdit::setTabDistance(q, 2)).
constexpr int kTabWidth = 2;
/// Сколько страниц кэшируем (видимая + соседние).
constexpr int kMaxCachedPages = 4;

struct IndexResult {
	bool ok = false;
	bool cancelled = false;
	QString error;
	QVector<qint64> lineStarts;
	int maxLineLength = 0;
	qint64 fileSize = 0;
};

/// Индексация файла: смещения начал строк. Байтовый поиск '\n' корректен
/// для UTF-8 и однобайтовых кодировок; UTF-16/UTF-32 прототип не берёт.
IndexResult buildIndex(const QString& path,
	const std::shared_ptr<std::atomic_bool>& cancel) {
	IndexResult result;

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		result.error = QObject::tr("Не удалось открыть файл: %1").arg(path);
		return result;
	}

	result.fileSize = file.size();
	result.lineStarts.reserve(
		static_cast<int>(qMin<qint64>(result.fileSize / 32 + 1, 20'000'000)));
	result.lineStarts.append(0);

	QByteArray buffer;
	buffer.resize(1024 * 1024);
	qint64 position = 0;
	qint64 lineStart = 0;
	int maxLineLength = 0;

	while (!file.atEnd()) {
		const qint64 read = file.read(buffer.data(), buffer.size());
		if (read <= 0) {
			break;
		}

		const char* data = buffer.constData();
		for (qint64 i = 0; i < read; i++) {
			if (data[i] != '\n') {
				continue;
			}

			const qint64 lineEnd = position + i;
			maxLineLength = qMax(maxLineLength, static_cast<int>(lineEnd - lineStart));
			lineStart = lineEnd + 1;
			result.lineStarts.append(lineStart);
		}

		position += read;
		if (cancel
			&& (position & 0xFFFFF) == 0
			&& cancel->load()) {
			result.cancelled = true;
			return result;
		}
	}

	maxLineLength = qMax(maxLineLength,
		static_cast<int>(result.fileSize - lineStart));
	result.maxLineLength = maxLineLength;
	result.ok = true;
	return result;
}

} // namespace

namespace {

struct SearchMatch {
	int line = 0;
	int start = 0;
	int length = 0;
};

struct SearchOutcome {
	bool cancelled = false;
	int total = 0;
	QVector<SearchMatch> matches;
};

/// Построчный поиск по файлу: страницы декодируются в фоне, поэтому
/// счётчик не блокирует окно. Многострочные шаблоны не поддержаны.
SearchOutcome runLineSearch(const QString& path, const QByteArray& encodingName,
	const QVector<qint64>& lineStarts, qint64 fileSize, const QString& query,
	Qt::CaseSensitivity caseSensitivity,
	const std::shared_ptr<std::atomic_bool>& cancel) {
	SearchOutcome outcome;
	if (query.isEmpty() || lineStarts.isEmpty()) {
		return outcome;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return outcome;
	}

	QTextCodec* codec = QTextCodec::codecForName(encodingName);
	if (!codec) {
		codec = QTextCodec::codecForName("UTF-8");
	}

	constexpr int kLinesPerChunk = 2000;
	constexpr int kMaxStored = 100000;
	for (int first = 0; first < lineStarts.size(); first += kLinesPerChunk) {
		if (cancel && cancel->load()) {
			outcome.cancelled = true;
			return outcome;
		}

		const int last = qMin(first + kLinesPerChunk, lineStarts.size());
		const qint64 begin = lineStarts.at(first);
		const qint64 end = last < lineStarts.size() ? lineStarts.at(last) : fileSize;
		file.seek(begin);
		const QByteArray bytes = file.read(qMax<qint64>(0, end - begin));
		QString text = codec ? codec->toUnicode(bytes) : QString::fromUtf8(bytes);
		QStringList lines = text.split(QLatin1Char('\n'));

		for (int i = 0; i < lines.size() && first + i < lineStarts.size(); i++) {
			QString line = lines.at(i);
			if (line.endsWith(QLatin1Char('\r'))) {
				line.chop(1);
			}

			int position = 0;
			while ((position = line.indexOf(query, position, caseSensitivity)) >= 0) {
				outcome.total++;
				if (outcome.matches.size() < kMaxStored) {
					outcome.matches.append({ first + i, position, int(query.size()) });
				}
				position += qMax(1, int(query.size()));
			}
		}
	}

	return outcome;
}

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

	struct Page {
		QStringList lines;
	};
	QHash<int, Page> pages;
	QList<int> pageOrder;

	QFutureWatcher<IndexResult>* watcher = nullptr;
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
	QVector<SearchMatch> searchMatches;
	int searchTotal = 0;
	int searchCurrent = -1;
	bool searching = false;
	bool searchSelectionActive = false;
	bool searchCaseSensitive = false;
	QFutureWatcher<SearchOutcome>* searchWatcher = nullptr;
	std::shared_ptr<std::atomic_bool> searchCancel;
	quint64 searchGeneration = 0;
	quint64 searchActiveGeneration = 0;

	void clearPages();
	void loadPage(int line);
	QString lineAt(int line) const;
	void updateScrollBars();
	int pageSize() const { return kPageLines; }

	int lineHeight() const;
	int charWidth() const;
	bool hasSelection() const;
	void normalizedSelection(int& startLine, int& startColumn,
		int& endLine, int& endColumn) const;
	QString selectedToken() const;
	int columnAt(int line, int x) const;
	void configureLayout(QTextLayout& layout) const;
	int gutterWidth() const;
	int logicalLineAtY(int y) const;
	bool positionAt(int x, int y, int& line, int& column) const;
	void ensureCursorVisible();
	void selectMatch(int index);
	void updateCurrentForCursor();
	void moveCursorVertically(int delta, bool keepAnchor);
};

void PagedTextEditor::Private::clearPages() {
	pages.clear();
	pageOrder.clear();
}

void PagedTextEditor::Private::loadPage(int line) {
	const int pageIndex = line / kPageLines;
	if (pages.contains(pageIndex) || !ready || line < 0 || line >= lines) {
		return;
	}

	const int firstLine = pageIndex * kPageLines;
	const int lastLine = qMin(lines, firstLine + kPageLines);
	if (firstLine >= lastLine) {
		return;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}

	const qint64 begin = lineStarts.at(firstLine);
	const qint64 end = lastLine < lineStarts.size()
		? lineStarts.at(lastLine) : fileSize;
	file.seek(begin);
	const QByteArray bytes = file.read(qMax<qint64>(0, end - begin));

	QTextCodec* pageCodec = codec ? codec : QTextCodec::codecForName("UTF-8");
	QString text = pageCodec ? pageCodec->toUnicode(bytes) : QString::fromUtf8(bytes);
	if (firstLine == 0 && text.startsWith(QChar(0xFEFF))) {
		text.remove(0, 1);
	}

	QStringList pageLines = text.split(QLatin1Char('\n'));
	for (QString& lineText : pageLines) {
		if (lineText.endsWith(QLatin1Char('\r'))) {
			lineText.chop(1);
		}
	}

	const int expected = lastLine - firstLine;
	if (pageLines.size() > expected) {
		pageLines = pageLines.mid(0, expected);
	}
	while (pageLines.size() < expected) {
		pageLines.append(QString());
	}

	pages.insert(pageIndex, Page{ pageLines });
	pageOrder.append(pageIndex);
	while (pageOrder.size() > kMaxCachedPages) {
		pages.remove(pageOrder.takeFirst());
	}
}

QString PagedTextEditor::Private::lineAt(int line) const {
	if (!ready || line < 0 || line >= lines) {
		return QString();
	}

	const int pageIndex = line / kPageLines;
	auto it = pages.constFind(pageIndex);
	if (it == pages.cend()) {
		const_cast<Private*>(this)->loadPage(line);
		it = pages.constFind(pageIndex);
	}
	if (it == pages.cend()) {
		return QString();
	}

	const int index = line - pageIndex * kPageLines;
	return it->lines.value(index);
}

int PagedTextEditor::Private::lineHeight() const {
	return qMax(1, q->fontMetrics().height());
}

int PagedTextEditor::Private::charWidth() const {
	return qMax(1, q->fontMetrics().horizontalAdvance(QLatin1Char('M')));
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

int PagedTextEditor::Private::columnAt(int line, int x) const {
	const QString text = lineAt(line);
	QTextLayout layout(text, q->font());
	configureLayout(layout);
	const QTextLine textLine = layout.lineAt(0);
	if (!textLine.isValid()) {
		return 0;
	}

	const int column = textLine.xToCursor(
		x + q->horizontalScrollBar()->value());
	return qBound(0, column, text.size());
}

void PagedTextEditor::Private::configureLayout(QTextLayout& layout) const {
	QTextOption option;
	option.setWrapMode(wrapLines ? QTextOption::WrapAtWordBoundaryOrAnywhere
		: QTextOption::NoWrap);
	option.setTabStopDistance(charWidth() * kTabWidth);
	layout.setTextOption(option);
	layout.beginLayout();
	const qreal width = wrapLines
		? qMax(1, q->viewport()->width() - gutterWidth() - 8) : 1e9;
	while (true) {
		QTextLine line = layout.createLine();
		if (!line.isValid()) {
			break;
		}
		line.setLineWidth(width);
		if (!wrapLines) {
			break;
		}
	}
	layout.endLayout();
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
	int top = 0;
	for (int i = first; i < lines; i++) {
		QTextLayout layout(lineAt(i), q->font());
		configureLayout(layout);
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
	int top = 0;
	for (int i = first; i < lines; i++) {
		const QString text = lineAt(i);
		QTextLayout layout(text, q->font());
		configureLayout(layout);
		const int count = qMax(1, layout.lineCount());
		for (int visual = 0; visual < count; visual++) {
			const QTextLine textLine = layout.lineAt(visual);
			const int height = textLine.isValid()
				? qMax(1, qCeil(textLine.height())) : q->fontMetrics().height();
			if (y < top + height) {
				line = i;
				column = textLine.isValid()
					? qBound(0, textLine.xToCursor(contentX), text.size()) : 0;
				return true;
			}
			top += height;
		}
		if (top > q->viewport()->height()) {
			break;
		}
	}
	return false;
}

void PagedTextEditor::Private::ensureCursorVisible() {
	auto* vertical = q->verticalScrollBar();
	const int pageStep = qMax(1, vertical->pageStep());
	if (cursorLine < vertical->value()) {
		vertical->setValue(cursorLine);
	}
	else if (cursorLine >= vertical->value() + pageStep) {
		vertical->setValue(cursorLine - pageStep + 2);
	}
}

void PagedTextEditor::Private::moveCursorVertically(int delta, bool keepAnchor) {
	if (!ready || lines <= 0 || delta == 0) {
		return;
	}

	const int line = cursorLine;
	QTextLayout layout(lineAt(line), q->font());
	configureLayout(layout);
	if (layout.lineCount() <= 0) {
		return;
	}

	int visual = -1;
	qreal cursorX = 0;
	for (int i = 0; i < layout.lineCount(); i++) {
		const QTextLine textLine = layout.lineAt(i);
		const int end = textLine.textStart() + textLine.textLength();
		if (cursorColumn >= textLine.textStart()
			&& (cursorColumn < end
				|| (i + 1 == layout.lineCount() && cursorColumn == end))) {
			visual = i;
			cursorX = textLine.cursorToX(cursorColumn);
			break;
		}
	}
	if (visual < 0) {
		return;
	}

	int targetLine = line;
	int targetVisual = visual + delta;
	if (targetVisual < 0) {
		targetLine = line - 1;
		if (targetLine < 0) {
			return;
		}
	}
	else if (targetVisual >= layout.lineCount()) {
		targetLine = line + 1;
		if (targetLine >= lines) {
			return;
		}
	}

	QTextLayout targetLayout(lineAt(targetLine), q->font());
	configureLayout(targetLayout);
	if (targetLayout.lineCount() <= 0) {
		return;
	}

	if (targetLine != line) {
		targetVisual = delta < 0 ? targetLayout.lineCount() - 1 : 0;
	}

	targetVisual = qBound(0, targetVisual, targetLayout.lineCount() - 1);
	const QTextLine target = targetLayout.lineAt(targetVisual);
	const int targetColumn = qBound(0, target.xToCursor(cursorX),
		lineAt(targetLine).size());

	q->setCursorPosition(targetLine, targetColumn, keepAnchor);
}

void PagedTextEditor::Private::selectMatch(int index) {
	if (index < 0 || index >= searchMatches.size()) {
		return;
	}

	searchCurrent = index;
	searchSelectionActive = true;
	const auto& match = searchMatches.at(index);
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
	const qint64 contentWidth = static_cast<qint64>(maxLineLength) * charWidth;
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

	d->watcher = new QFutureWatcher<IndexResult>(this);
	connect(d->watcher, &QFutureWatcher<IndexResult>::finished, this, [this] {
		if (!d) {
			return;
		}

		d->indexing = false;
		const IndexResult result = d->watcher->result();
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
		d->ready = true;
		d->error.clear();
		d->clearPages();
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

	d->searchWatcher = new QFutureWatcher<SearchOutcome>(this);
	connect(d->searchWatcher, &QFutureWatcher<SearchOutcome>::finished, this, [this] {
		if (!d) {
			return;
		}

		d->searching = false;
		const SearchOutcome outcome = d->searchWatcher->result();
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

	d->updateScrollBars();
}

PagedTextEditor::~PagedTextEditor() {
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
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->clearPages();
	d->ready = false;
	d->indexing = false;
	d->error.clear();
	d->lines = 0;
	d->maxLineLength = 0;
	d->fileSize = 0;
	d->lineStarts.clear();
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
	viewport()->update();

	d->watcher->setFuture(QtConcurrent::run(
		[path, cancel] {
			return buildIndex(path, cancel);
		}));
}

void PagedTextEditor::closeFile() {
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	d->generation++;
	d->clearPages();
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

int PagedTextEditor::cursorVisualRow() const {
	if (!d->ready || d->cursorLine < 0 || d->cursorLine >= d->lines) {
		return 0;
	}

	QTextLayout layout(d->lineAt(d->cursorLine), font());
	d->configureLayout(layout);
	for (int visual = 0; visual < layout.lineCount(); visual++) {
		const QTextLine textLine = layout.lineAt(visual);
		const int end = textLine.textStart() + textLine.textLength();
		if (d->cursorColumn >= textLine.textStart()
			&& (d->cursorColumn < end
				|| (visual + 1 == layout.lineCount() && d->cursorColumn == end))) {
			return visual;
		}
	}
	return 0;
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
		emit searchCountChanged(0);
		return;
	}

	emit searchCountChanged(-1);
	auto cancel = std::make_shared<std::atomic_bool>(false);
	d->searchCancel = cancel;
	d->searchActiveGeneration = d->searchGeneration;
	d->searching = true;

	d->searchWatcher->setFuture(QtConcurrent::run(
		[path = d->path, encoding = d->encodingName.toLatin1(),
			starts = d->lineStarts, size = d->fileSize, query,
			caseSensitivity = d->searchCaseSensitive ? Qt::CaseSensitive
				: Qt::CaseInsensitive, cancel] {
			return runLineSearch(path, encoding, starts, size, query,
				caseSensitivity, cancel);
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
	QPainter painter(viewport());
	painter.fillRect(event->rect(), palette().base());

	if (d->indexing) {
		painter.setPen(palette().text().color());
		painter.drawText(viewport()->rect(), Qt::AlignCenter,
			tr("Индексация файла…"));
		return;
	}

	if (!d->ready) {
		painter.setPen(palette().placeholderText().color());
		painter.drawText(viewport()->rect().adjusted(8, 8, -8, -8),
			Qt::AlignLeft | Qt::AlignTop, d->error);
		return;
	}

	const QFontMetrics metrics = painter.fontMetrics();
	const int lineHeight = qMax(1, metrics.height());
	const int gutter = d->gutterWidth();
	const int first = verticalScrollBar()->value();
	const int viewportWidth = viewport()->width();
	const int viewportHeight = viewport()->height();
	const int horizontalOffset = d->wrapLines ? 0 : horizontalScrollBar()->value();

	if (gutter > 0) {
		painter.fillRect(QRect(0, 0, gutter, viewportHeight),
			palette().alternateBase());
		painter.setPen(palette().mid().color());
		painter.drawLine(gutter - 1, 0, gutter - 1, viewportHeight);
		painter.setPen(palette().placeholderText().color());
	}

	const QColor selectionColor = palette().highlight().color();
	const QColor tokenColor = palette().highlight().color();
	const QColor searchColor = palette().highlight().color();
	const QColor currentSearchColor = palette().highlight().color();

	bool hasSelection = d->hasSelection();
	int startLine = 0;
	int startColumn = 0;
	int endLine = 0;
	int endColumn = 0;
	if (hasSelection) {
		d->normalizedSelection(startLine, startColumn, endLine, endColumn);
	}

	const QString token = d->selectedToken();
	const bool highlightToken = !token.isEmpty() && token.size() >= 2;

	painter.save();
	int y = 0;
	for (int i = first; i < d->lines && y < viewportHeight; i++) {
		QString text = d->lineAt(i);
		if (text.endsWith(QLatin1Char('\r'))) {
			text.chop(1);
		}

		QTextLayout textLayout(text, painter.font());
		d->configureLayout(textLayout);
		const int visualCount = qMax(1, textLayout.lineCount());
		bool firstVisual = true;

		for (int visual = 0; visual < visualCount && y < viewportHeight; visual++) {
			const QTextLine textLine = textLayout.lineAt(visual);
			const int visualHeight = textLine.isValid()
				? qMax(1, qCeil(textLine.height())) : lineHeight;
			if (!textLine.isValid()) {
				if (firstVisual) {
					painter.setPen(palette().placeholderText().color());
					painter.drawText(QRectF(0, y, gutter - 8, visualHeight),
						Qt::AlignRight | Qt::AlignVCenter, QString::number(i + 1));
					firstVisual = false;
				}
				y += visualHeight;
				continue;
			}

			const int segmentStart = textLine.textStart();
			const int segmentEnd = segmentStart + textLine.textLength();
			// QTextLine::cursorToX() принимает абсолютную позицию в тексте
			// строки, поэтому не вычитаем начало сегмента.
			const auto xAt = [&textLine](int column) {
				return textLine.cursorToX(column);
			};

			// Все вхождения выделенного токена (как в VS Code).
			if (highlightToken) {
				int position = text.indexOf(token, 0, Qt::CaseSensitive);
				while (position >= 0) {
					const int from = qMax(position, segmentStart);
					const int to = qMin(position + token.size(), segmentEnd);
					if (to > from) {
						QColor color = tokenColor;
						color.setAlpha(80);
						const int x1 = qRound(xAt(from));
						const int x2 = qRound(xAt(to));
						painter.fillRect(gutter - horizontalOffset + x1, y,
							x2 - x1, visualHeight, color);
					}
					position = text.indexOf(token, position + token.size(),
						Qt::CaseSensitive);
				}
			}

			// Найденные совпадения — только на видимых строках.
			if (!d->searchMatches.isEmpty() && i >= d->searchMatches.first().line) {
				auto match = std::lower_bound(d->searchMatches.cbegin(),
					d->searchMatches.cend(), i,
					[](const SearchMatch& value, int line) { return value.line < line; });
				for (; match != d->searchMatches.cend() && match->line == i; ++match) {
					const int from = qMax(match->start, segmentStart);
					const int to = qMin(match->start + match->length, segmentEnd);
					if (to <= from) {
						continue;
					}
					const int index = static_cast<int>(match - d->searchMatches.cbegin());
					QColor color = index == d->searchCurrent
						? currentSearchColor : searchColor;
					color.setAlpha(index == d->searchCurrent ? 220 : 120);
					const int x1 = qRound(xAt(from));
					const int x2 = qRound(xAt(to));
					painter.fillRect(gutter - horizontalOffset + x1, y,
						x2 - x1, visualHeight, color);
				}
			}

			// Выделение мышью/клавиатурой.
			if (hasSelection && i >= startLine && i <= endLine) {
				const int selectedFrom = i == startLine ? startColumn : 0;
				const int selectedTo = i == endLine ? endColumn : text.size();
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

			painter.setPen(palette().text().color());
			textLine.draw(&painter, QPointF(gutter - horizontalOffset, y));

			if (hasFocus() && d->ready && i == d->cursorLine
				&& d->cursorColumn >= segmentStart
				&& (d->cursorColumn < segmentEnd
					|| (visual + 1 == visualCount && d->cursorColumn == segmentEnd))) {
				const qreal cursorX = xAt(d->cursorColumn);				painter.setPen(palette().text().color());
				painter.drawLine(QPointF(gutter - horizontalOffset + cursorX, y),
					QPointF(gutter - horizontalOffset + cursorX, y + visualHeight));
			}

			if (firstVisual && gutter > 0) {
				painter.setPen(palette().placeholderText().color());
				painter.drawText(QRectF(0, y, gutter - 8, visualHeight),
					Qt::AlignRight | Qt::AlignVCenter, QString::number(i + 1));
				firstVisual = false;
			}

			y += visualHeight;
		}
	}

	painter.restore();
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
