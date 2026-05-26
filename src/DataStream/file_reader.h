#pragma once

#include <QObject>
#include <QStringList>

class FileReader : public QObject {
	Q_OBJECT

public:
	explicit FileReader(QObject* parent = nullptr);

public slots:
	void readFileAsync(const QString& filePath);

signals:
	void lineRead(const QString& line);
	void finished();
};
