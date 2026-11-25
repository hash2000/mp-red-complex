#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_search.h"

PagedSearchOutcome runPagedLineSearch(PagedDocumentBuffer buffer,
	const QString& query, Qt::CaseSensitivity caseSensitivity,
	const std::shared_ptr<std::atomic_bool>& cancel,
	const std::function<void(int)>& progress) {
	PagedSearchOutcome outcome;
	const int lines = buffer.lineCount();
	if (query.isEmpty() || lines <= 0) {
		return outcome;
	}

	constexpr int kLinesPerChunk = 2000;
	constexpr int kMaxStored = 100000;
	const int step = qMax(1, int(query.size()));
	for (int first = 0; first < lines; first += kLinesPerChunk) {
		if (cancel && cancel->load()) {
			outcome.cancelled = true;
			return outcome;
		}

		const int last = qMin(first + kLinesPerChunk, lines);
		for (int line = first; line < last; line++) {
			const QString text = buffer.lineAt(line);
			int position = 0;
			while ((position = text.indexOf(query, position, caseSensitivity)) >= 0) {
				outcome.total++;
				if (outcome.matches.size() < kMaxStored) {
					outcome.matches.append({ line, position, static_cast<int>(query.size()) });
				}
				position += step;
			}
		}

		if (progress) {
			progress(outcome.total);
		}
	}

	return outcome;
}
