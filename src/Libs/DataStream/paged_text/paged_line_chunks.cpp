#include "Libs/DataStream/paged_text/paged_line_chunks.h"

#include <QtGlobal>

void PagedLineChunks::clear() {
	loadedChunks.clear();
}

QString PagedLineChunks::displayedText(const QString& lineText, int line,
	bool wrapped, int maxLineLength) const {
	if (wrapped) {
		// Перенос строк показывается целиком: у него нет горизонтальной
		// прокрутки, поэтому куски нельзя догрузить жестом.
		return lineText;
	}
	const int currentLimit = limit(line, wrapped, maxLineLength);
	return lineText.size() > currentLimit ? lineText.left(currentLimit) : lineText;
}

int PagedLineChunks::limit(int line, bool wrapped, int maxLineLength) const {
	if (wrapped) {
		return qMax(0, maxLineLength);
	}
	const int chunks = 1 + qMax(0, loadedChunks.value(line, 0));
	return qMin(kUnwrappedChunk * chunks, qMax(0, maxLineLength));
}

int PagedLineChunks::maximumLimit(bool wrapped, int maxLineLength) const {
	if (wrapped) {
		return qMax(0, maxLineLength);
	}
	int extra = 0;
	for (auto it = loadedChunks.cbegin(); it != loadedChunks.cend(); ++it) {
		extra = qMax(extra, it.value());
	}
	return qMin(kUnwrappedChunk * (1 + extra),
		qMax(kUnwrappedChunk, maxLineLength));
}

void PagedLineChunks::loadMore(int line) {
	loadedChunks[line] = loadedChunks.value(line, 0) + 1;
}

void PagedLineChunks::ensureLimit(int line, int needed, bool wrapped,
	int maxLineLength) {
	if (wrapped) {
		return;
	}
	while (limit(line, wrapped, maxLineLength) < needed
		&& limit(line, wrapped, maxLineLength) < maxLineLength) {
		loadMore(line);
	}
}
