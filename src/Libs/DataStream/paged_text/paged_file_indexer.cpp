#include "Libs/DataStream/paged_text/paged_file_indexer.h"

#include <QCoreApplication>
#include <QFile>

PagedIndexResult indexPagedFile(const QString& path,
	const std::shared_ptr<std::atomic_bool>& cancel) {
	PagedIndexResult result;

	QFile file(path);
	if (!file.open(QIODevice::ReadOnly)) {
		result.error = QCoreApplication::translate(
			"PagedFileIndexer", "Не удалось открыть файл: %1").arg(path);
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
			maxLineLength = qMax(maxLineLength,
				static_cast<int>(lineEnd - lineStart));
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
