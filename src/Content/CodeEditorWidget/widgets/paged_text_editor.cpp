#include "Content/CodeEditorWidget/widgets/paged_text_editor.h"

#include <QFile>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QHash>
#include <QKeyEvent>
#include <QList>
#include <QPainter>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextCodec>
#include <QtConcurrent/QtConcurrentRun>

#include <atomic>
#include <limits>
#include <memory>

namespace {

/// Сколько строк держим в одной странице.
constexpr int kPageLines = 2000;
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

	void clearPages();
	void loadPage(int line);
	QString lineAt(int line) const;
	void updateScrollBars();
	int pageSize() const { return kPageLines; }
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
		verticalScrollBar()->setValue(0);
		viewport()->update();
		emit lineCountChanged(d->lines);
		emit indexingFinished(true, QString());
	});

	d->updateScrollBars();
}

PagedTextEditor::~PagedTextEditor() {
	if (d->cancelFlag) {
		d->cancelFlag->store(true);
	}
	if (d->watcher) {
		d->watcher->waitForFinished();
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

	painter.save();
	painter.translate(-horizontalScrollBar()->value(), 0);
	painter.setPen(palette().text().color());
	for (int i = first; i < d->lines && i < first + visibleCount; i++) {
		QString text = d->lineAt(i);
		if (text.endsWith(QLatin1Char('\r'))) {
			text.chop(1);
		}

		const int y = (i - first) * lineHeight;
		painter.drawText(0, y + metrics.ascent(), text);
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

void PagedTextEditor::keyPressEvent(QKeyEvent* event) {
	const int pageStep = qMax(1,
		viewport()->height() / qMax(1, fontMetrics().height()) - 1);
	auto* vertical = verticalScrollBar();
	switch (event->key()) {
	case Qt::Key_Up:
		vertical->setValue(vertical->value() - 1);
		return;
	case Qt::Key_Down:
		vertical->setValue(vertical->value() + 1);
		return;
	case Qt::Key_PageUp:
		vertical->setValue(vertical->value() - pageStep);
		return;
	case Qt::Key_PageDown:
		vertical->setValue(vertical->value() + pageStep);
		return;
	case Qt::Key_Home:
		vertical->setValue(0);
		return;
	case Qt::Key_End:
		vertical->setValue(vertical->maximum());
		return;
	default:
		break;
	}

	QAbstractScrollArea::keyPressEvent(event);
}
