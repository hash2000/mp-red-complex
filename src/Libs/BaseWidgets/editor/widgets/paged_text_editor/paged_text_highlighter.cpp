#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_highlighter.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"

void PagedTextHighlighter::setPlugin(HighlightingPlugin* plugin) {
	highlightingPlugin = plugin;
}

HighlightingPlugin* PagedTextHighlighter::plugin() const {
	return highlightingPlugin;
}

QVector<PagedTextSpan> PagedTextHighlighter::spansForLine(const QString& line) const {
	QVector<PagedTextSpan> spans;
	if (!highlightingPlugin || line.isEmpty()) {
		return spans;
	}

	highlightingPlugin->visitMatches(line,
		[&spans, length = line.size()](qsizetype start, qsizetype spanLength,
			const QTextCharFormat& format, int priority) {
			Q_UNUSED(priority);
			const int from = qBound(0, int(start), length);
			const int to = qBound(0, int(start + spanLength), length);
			if (to > from) {
				spans.append({ from, to - from, format });
			}
		});
	return spans;
}
