#include "Libs/DataStream/file_reader.h"
#include <QThread>
#include <QTextStream>
#include <QFile>

FileReader::FileReader(QObject* parent)
	: QObject(parent) {
}

void FileReader::setChunkSize(int lines) {
	_chunkSize = lines;
}

void FileReader::readFileAsync(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream stream(&file);
	QStringList buffer;
	buffer.reserve(_chunkSize);

	while (!stream.atEnd()) {
		QString line = stream.readLine();
		buffer.append(line);

		if (buffer.size() >= _chunkSize) {
			emit blockRead(buffer);
			buffer.clear();
			QThread::msleep(10);
		}
	}

	if (!buffer.isEmpty()) {
		emit blockRead(buffer);
	}

	emit finished();
}
