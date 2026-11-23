#pragma once

#include <QObject>
#include <QStringConverter>
#include <QStringList>

class FileReader : public QObject {
	Q_OBJECT

public:
	explicit FileReader(QObject* parent = nullptr);

	void setChunkSize(int lines);
	/// Кодировка файла при чтении (по умолчанию UTF-8).
	void setEncoding(QStringConverter::Encoding encoding);

public slots:
	void readFileAsync(const QString& filePath);

signals:
	void blockRead(const QStringList& line);
	/// Файл не удалось открыть; после сигнала приходит finished().
	void failed(const QString& errorMessage);
	void finished();

private:
	int _chunkSize = 500;
	QStringConverter::Encoding _encoding = QStringConverter::Utf8;
};
