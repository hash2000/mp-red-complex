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

	/// Метрики, посчитанные последним чтением (валидны после finished()).
	int lineCount() const;
	qint64 characterCount() const;
	int maxLineLength() const;

public slots:
	void readFileAsync(const QString& filePath);

signals:
	void blockRead(const QStringList& line);
	/// Метрики файла, посчитанные в фоне при чтении: строки, символы,
	/// длина самой длинной строки (без перевода строки).
	void metricsComputed(int lines, qint64 characters, int maxLineLength);
	/// Файл не удалось открыть; после сигнала приходит finished().
	void failed(const QString& errorMessage);
	void finished();

private:
	int _chunkSize = 500;
	QString _encoding = QStringLiteral("UTF-8");

	int _lineCount = 0;
	qint64 _characterCount = 0;
	int _maxLineLength = 0;
};
