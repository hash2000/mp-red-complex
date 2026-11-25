#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_line_cache.h"

#include <QFile>
#include <QTextCodec>

PagedLineCache::PagedLineCache(int pageLines, int maxCachedPages)
	: linesPerPage(qMax(1, pageLines))
	, maxCachedPages(qMax(1, maxCachedPages)) {
}

void PagedLineCache::reset(const QString& path, const QByteArray& encodingName,
	const QVector<qint64>& lineStarts, qint64 fileSize) {
	this->path = path;
	this->encodingName = encodingName;
	this->lineStarts = lineStarts;
	this->fileSize = fileSize;
	clear();
}

void PagedLineCache::clear() {
	pages.clear();
	pageOrder.clear();
}

void PagedLineCache::loadPage(int line) const {
	const int pageIndex = line / linesPerPage;
	if (pages.contains(pageIndex) || line < 0 || line >= lineStarts.size()) {
		return;
	}

	const int firstLine = pageIndex * linesPerPage;
	const int lastLine = qMin(lineStarts.size(), firstLine + linesPerPage);
	if (firstLine >= lastLine) {
		return;
	}

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		return;
	}

	const qint64 begin = lineStarts.at(firstLine);
	const qint64 end = lastLine < lineStarts.size() ? lineStarts.at(lastLine) : fileSize;
	file.seek(begin);
	const QByteArray bytes = file.read(qMax<qint64>(0, end - begin));

	QTextCodec* codec = QTextCodec::codecForName(encodingName);
	if (!codec) {
		codec = QTextCodec::codecForName("UTF-8");
	}
	QString text = codec ? codec->toUnicode(bytes) : QString::fromUtf8(bytes);
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
	while (pageOrder.size() > maxCachedPages) {
		pages.remove(pageOrder.takeFirst());
	}
}

QString PagedLineCache::lineAt(int line) const {
	if (line < 0 || line >= lineStarts.size()) {
		return QString();
	}

	const int pageIndex = line / linesPerPage;
	auto it = pages.constFind(pageIndex);
	if (it == pages.cend()) {
		loadPage(line);
		it = pages.constFind(pageIndex);
	}
	if (it == pages.cend()) {
		return QString();
	}

	return it->lines.value(line - pageIndex * linesPerPage);
}

int PagedLineCache::pageLines() const {
	return linesPerPage;
}
