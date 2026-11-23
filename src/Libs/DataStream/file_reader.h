#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class FileReader : public QObject {
	Q_OBJECT

public:
	explicit FileReader(QObject* parent = nullptr);

	void setChunkSize(int lines);
	/// Кодировка файла при чтении (имя кодека QTextCodec, по умолчанию UTF-8).
	void setEncoding(const QString& encoding);

public slots:
	void readFileAsync(const QString& filePath);

signals:
	void blockRead(const QStringList& line);
	/// Файл не удалось открыть; после сигнала приходит finished().
	void failed(const QString& errorMessage);
	void finished();

private:
	int _chunkSize = 500;
	QString _encoding = QStringLiteral("UTF-8");
};
