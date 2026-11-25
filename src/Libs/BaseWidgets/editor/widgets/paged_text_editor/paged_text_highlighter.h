#pragma once
#include <QString>
#include <QTextCharFormat>
#include <QVector>

class HighlightingPlugin;

/// Фрагмент подсветки одной строки: границы в символах и формат.
struct PagedTextSpan {
	int start = 0;
	int length = 0;
	QTextCharFormat format;
};

/// Построчная подсветка для paged-редактора.
///
/// Использует правила HighlightingPlugin, но не QSyntaxHighlighter: редактор
/// не держит QTextDocument, поэтому подсвечиваются только видимые строки.
class PagedTextHighlighter {
public:
	void setPlugin(HighlightingPlugin* plugin);
	HighlightingPlugin* plugin() const;

	QVector<PagedTextSpan> spansForLine(const QString& line) const;

private:
	HighlightingPlugin* highlightingPlugin = nullptr;
};
