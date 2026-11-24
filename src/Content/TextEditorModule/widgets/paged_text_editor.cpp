#include "Content/TextEditorModule/widgets/paged_text_editor.h"

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
#include <QtConcurrent/QtConcurrentRun>

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
	void ensureCursorVisible();
	void selectMatch(int index);
	void updateCurrentForCursor();
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
	option.setWrapMode(QTextOption::NoWrap);
	option.setTabStopDistance(charWidth() * kTabWidth);
	layout.setTextOption(option);
	layout.beginLayout();
	QTextLine line = layout.createLine();
	if (line.isValid()) {
		line.setLineWidth(1e9);
	}
	layout.endLayout();
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
	const int maxHorizontal = static_cast<int>(qMin<qint64>(
		qMax<qint64>(0, contentWidth - q->viewport()->width()),
		std::numeric_limits<int>::max()));
	auto* horizontal = q->horizontalScrollBar();
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
	const int first = verticalScrollBar()->value();
	const int visibleCount = viewport()->height() / lineHeight + 2;
	const int lastLine = qMin(d->lines, first + visibleCount);

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
	painter.translate(-horizontalScrollBar()->value(), 0);

	for (int i = first; i < lastLine; i++) {
		QString text = d->lineAt(i);
		if (text.endsWith(QLatin1Char('\r'))) {
			text.chop(1);
		}

		const int y = (i - first) * lineHeight;

		// Раскладка строки: те же координаты используются для отрисовки,
		// выделения, подсветки и попадания мыши.
		QTextLayout textLayout(text, painter.font());
		d->configureLayout(textLayout);
		const QTextLine textLine = textLayout.lineAt(0);
		const auto xAt = [&textLine, &text](int column) {
			if (!textLine.isValid()) {
				return 0.0;
			}
			return textLine.cursorToX(qBound(0, column, text.size()));
		};

		// Все вхождения выделенного токена (как в VS Code).
		if (highlightToken) {
			int position = 0;
			while ((position = text.indexOf(token, position, Qt::CaseSensitive)) >= 0) {
				QColor color = tokenColor;
				color.setAlpha(80);
				const int x1 = qRound(xAt(position));
				const int x2 = qRound(xAt(position + token.size()));
				painter.fillRect(x1, y, x2 - x1, lineHeight, color);
				position += token.size();
			}
		}

		// Найденные совпадения — только на видимых строках.
		if (!d->searchMatches.isEmpty() && i >= d->searchMatches.first().line) {
			auto match = std::lower_bound(d->searchMatches.cbegin(),
				d->searchMatches.cend(), i,
				[](const SearchMatch& value, int line) { return value.line < line; });
			for (; match != d->searchMatches.cend() && match->line == i; ++match) {
				const int index = static_cast<int>(match - d->searchMatches.cbegin());
				QColor color = index == d->searchCurrent ? currentSearchColor : searchColor;
				color.setAlpha(index == d->searchCurrent ? 220 : 120);
				const int x1 = qRound(xAt(match->start));
				const int x2 = qRound(xAt(match->start + match->length));
				painter.fillRect(x1, y, x2 - x1, lineHeight, color);
			}
		}

		// Выделение мышью/клавиатурой.
		if (hasSelection && i >= startLine && i <= endLine) {
			const int from = i == startLine ? startColumn : 0;
			const int to = i == endLine ? endColumn : text.size();
			if (to > from) {
				QColor color = selectionColor;
				color.setAlpha(170);
				const int x1 = qRound(xAt(from));
				const int x2 = qRound(xAt(to));
				painter.fillRect(x1, y, x2 - x1, lineHeight, color);
			}
		}

		painter.setPen(palette().text().color());
		textLayout.draw(&painter, QPointF(0, y));
	}

	if (hasFocus() && d->ready) {
		const int cursorY = (d->cursorLine - first) * lineHeight;
		if (cursorY >= 0 && cursorY < viewport()->height()) {
			QTextLayout cursorLayout(d->lineAt(d->cursorLine), painter.font());
			d->configureLayout(cursorLayout);
			const QTextLine cursorLine = cursorLayout.lineAt(0);
			const qreal cursorX = cursorLine.isValid()
				? cursorLine.cursorToX(d->cursorColumn) : 0.0;
			painter.setPen(palette().text().color());
			painter.drawLine(QPointF(cursorX, cursorY),
				QPointF(cursorX, cursorY + lineHeight));
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

void PagedTextEditor::mousePressEvent(QMouseEvent* event) {
	if (event->button() != Qt::LeftButton || !d->ready) {
		QAbstractScrollArea::mousePressEvent(event);
		return;
	}

	setFocus(Qt::MouseFocusReason);
	const int lineHeight = d->lineHeight();
	const int line = qBound(0, verticalScrollBar()->value()
		+ int(event->position().y()) / lineHeight, qMax(0, d->lines - 1));
	const int column = d->columnAt(line, int(event->position().x()));

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

	const int lineHeight = d->lineHeight();
	int line = verticalScrollBar()->value() + int(event->position().y()) / lineHeight;
	if (event->position().y() < 0 && verticalScrollBar()->value() > 0) {
		verticalScrollBar()->setValue(verticalScrollBar()->value() - 1);
	}
	else if (event->position().y() > viewport()->height()) {
		verticalScrollBar()->setValue(verticalScrollBar()->value() + 1);
	}

	line = qBound(0, line, qMax(0, d->lines - 1));
	const int column = d->columnAt(line, int(event->position().x()));
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

	const int lineHeight = d->lineHeight();
	const int line = qBound(0, verticalScrollBar()->value()
		+ int(event->position().y()) / lineHeight, qMax(0, d->lines - 1));
	QString text = d->lineAt(line);
	int start = qMin(d->columnAt(line, int(event->position().x())), text.size());
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
		setCursorPosition(d->cursorLine - 1,
			qMin(d->cursorColumn, d->lineAt(qMax(0, d->cursorLine - 1)).size()),
			shift);
		return;
	case Qt::Key_Down:
		setCursorPosition(d->cursorLine + 1,
			qMin(d->cursorColumn, d->lineAt(qMin(d->lines - 1,
				d->cursorLine + 1)).size()), shift);
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
