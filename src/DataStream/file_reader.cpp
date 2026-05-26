#include "DataStream/file_reader.h"
#include <QThread>
#include <QTextStream>
#include <QFile>

FileReader::FileReader(QObject* parent)
	: QObject(parent) {
}

void FileReader::readFileAsync(const QString& filePath) {
	QFile file(filePath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream stream(&file);
	while (!stream.atEnd()) {
		QString line = stream.readLine();
		emit lineRead(line);
		QThread::msleep(10);
	}

	emit finished();
}
