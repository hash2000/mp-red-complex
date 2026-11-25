#pragma once
#include <QString>
#include <QVector>

#include <atomic>
#include <memory>

/// Результат индексации файла: смещения строк и максимальная длина строки.
struct PagedIndexResult {
	bool ok = false;
	bool cancelled = false;
	QString error;
	QVector<qint64> lineStarts;
	int maxLineLength = 0;
	qint64 fileSize = 0;
};

/// Индексация файла в фоне: один проход по байтам, без загрузки строк в память.
PagedIndexResult indexPagedFile(const QString& path,
	const std::shared_ptr<std::atomic_bool>& cancel);
