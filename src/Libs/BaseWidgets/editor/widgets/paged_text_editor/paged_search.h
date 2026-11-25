#pragma once
#include <QByteArray>
#include <QString>
#include <QVector>

#include <atomic>
#include <functional>
#include <memory>

/// Совпадение в paged-редакторе (строка и колонки).
struct PagedSearchMatch {
	int line = 0;
	int start = 0;
	int length = 0;
};

struct PagedSearchOutcome {
	bool cancelled = false;
	int total = 0;
	QVector<PagedSearchMatch> matches;
};

/// Построчный поиск по файлу. Страницы декодируются в фоне, поэтому
/// счётчик не блокирует GUI. Многострочные шаблоны не поддержаны.
PagedSearchOutcome runPagedLineSearch(const QString& path,
	const QByteArray& encodingName, const QVector<qint64>& lineStarts,
	qint64 fileSize, const QString& query, Qt::CaseSensitivity caseSensitivity,
	const std::shared_ptr<std::atomic_bool>& cancel,
	const std::function<void(int)>& progress = {});
