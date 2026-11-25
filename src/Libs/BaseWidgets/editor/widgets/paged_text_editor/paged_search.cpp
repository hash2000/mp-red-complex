#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_search.h"

#include <QFile>
#include <QStringList>
#include <QTextCodec>

PagedSearchOutcome runPagedLineSearch(const QString& path,
	const QByteArray& encodingName, const QVector<qint64>& lineStarts,
	qint64 fileSize, const QString& query, Qt::CaseSensitivity caseSensitivity,
	const std::shared_ptr<std::atomic_bool>& cancel,
	const std::function<void(int)>& progress) {
	PagedSearchOutcome outcome;
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

		if (progress) {
			progress(outcome.total);
		}
	}

	return outcome;
}
