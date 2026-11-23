#include "Libs/DataStream/file_reader.h"
#include <QDebug>
#include <QFile>
#include <QTextCodec>
#include <QThread>
#include <memory>

namespace {

/// Размер порции чтения: достаточно крупно, чтобы не гонять файл по байту,
/// и достаточно мелко, чтобы не держать в памяти весь файл целиком.
constexpr int kReadChunkSize = 64 * 1024;

} // namespace

FileReader::FileReader(QObject* parent)
	: QObject(parent) {
}

void FileReader::setChunkSize(int lines) {
	_chunkSize = lines;
}

void FileReader::setEncoding(const QString& encoding) {
	_encoding = encoding;
}

void FileReader::readFileAsync(const QString& filePath) {
	QFile file(filePath);
	// Файл читается как двоичный: переводы строк и кодировку разбираем сами.
	if (!file.open(QIODevice::ReadOnly)) {
		emit failed(tr("Не удалось открыть файл: %1").arg(filePath));
		emit finished();
		return;
	}

	QTextCodec* codec = QTextCodec::codecForName(_encoding.toLatin1());
	if (!codec) {
		qWarning() << "FileReader: unknown encoding" << _encoding << "- using UTF-8";
		codec = QTextCodec::codecForName("UTF-8");
	}

	std::unique_ptr<QTextDecoder> decoder;
	if (codec) {
		decoder.reset(codec->makeDecoder());
	}

	if (!decoder) {
		emit failed(tr("Не удалось подобрать кодировку для файла: %1").arg(filePath));
		emit finished();
		return;
	}

	QStringList buffer;
	buffer.reserve(_chunkSize);
	QString pending;
	bool firstChunk = true;

	while (!file.atEnd()) {
		const QByteArray chunk = file.read(kReadChunkSize);
		if (chunk.isEmpty()) {
			break;
		}

		pending += decoder->toUnicode(chunk);
		if (firstChunk) {
			firstChunk = false;
			// BOM уже учтён кодеком, но некоторые кодеки оставляют U+FEFF.
			if (!pending.isEmpty() && pending.at(0) == QChar(0xFEFF)) {
				pending.remove(0, 1);
			}
		}

		int lineStart = 0;
		for (int i = 0; i < pending.size(); i++) {
			if (pending.at(i) != QLatin1Char('\n')) {
				continue;
			}

			int lineEnd = i;
			if (lineEnd > lineStart && pending.at(lineEnd - 1) == QLatin1Char('\r')) {
				lineEnd--;
			}

			buffer.append(pending.mid(lineStart, lineEnd - lineStart));
			lineStart = i + 1;

			if (buffer.size() >= _chunkSize) {
				emit blockRead(buffer);
				buffer.clear();
				QThread::msleep(10);
			}
		}

		pending.remove(0, lineStart);
	}

	// Последняя строка без завершающего перевода строки.
	if (!pending.isEmpty()) {
		if (pending.endsWith(QLatin1Char('\r'))) {
			pending.chop(1);
		}
		if (!pending.isEmpty()) {
			buffer.append(pending);
		}
	}

	if (!buffer.isEmpty()) {
		emit blockRead(buffer);
	}

	emit finished();
}
