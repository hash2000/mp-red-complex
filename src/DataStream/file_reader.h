#pragma once

#include <QObject>
#include <QStringList>

class FileReader : public QObject {
	Q_OBJECT

public:
	explicit FileReader(QObject* parent = nullptr);

	void setChunkSize(int lines);

public slots:
	void readFileAsync(const QString& filePath);

signals:
	void blockRead(const QStringList& line);
	void finished();

private:
	int _chunkSize = 500;
};
