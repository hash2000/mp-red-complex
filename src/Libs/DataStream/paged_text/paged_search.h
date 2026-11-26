#pragma once
#include "Libs/DataStream/paged_text/paged_document_buffer.h"

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

/// Построчный поиск по виртуальному буферу. Буфер передаётся по значению:
/// это снимок документа, в котором неизменённые страницы по-прежнему читаются
/// с диска, а изменённые берутся из памяти. Поэтому поиск не блокирует GUI
/// и учитывает несохранённые правки. Многострочные шаблоны не поддержаны.
PagedSearchOutcome runPagedLineSearch(PagedDocumentBuffer buffer,
	const QString& query, Qt::CaseSensitivity caseSensitivity,
	const std::shared_ptr<std::atomic_bool>& cancel,
	const std::function<void(int)>& progress = {});
