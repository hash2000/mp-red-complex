#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_edit_history.h"

void PagedEditHistory::clear() {
	undoStack.clear();
	redoStack.clear();
	totalCost = 0;
}

qint64 PagedEditHistory::costOf(const PagedEditRecord& record) {
	// Оценка объёма снимков: массив указателей на строки плюс сами строки.
	// Данные строк разделяются между снимками (QString — copy-on-write,
	// а правка всегда создаёт новую строку целиком), поэтому оценка сверху.
	const auto snapshotCost = [](const PagedPageSnapshot& snapshot) {
		qint64 cost = snapshot.lines.size() * static_cast<qint64>(sizeof(QString));
		for (const QString& line : snapshot.lines) {
			cost += line.size() * 2;
		}
		return cost;
	};

	qint64 cost = 0;
	for (const auto& entry : record.before) {
		cost += snapshotCost(entry.second);
	}
	for (const auto& entry : record.after) {
		cost += snapshotCost(entry.second);
	}
	return cost;
}

bool PagedEditHistory::samePages(const PagedEditRecord& left,
	const PagedEditRecord& right) {
	if (left.before.size() != right.before.size()) {
		return false;
	}
	for (int i = 0; i < left.before.size(); i++) {
		if (left.before.at(i).first != right.before.at(i).first) {
			return false;
		}
	}
	return true;
}

bool PagedEditHistory::samePosition(const PagedCursorState& left,
	const PagedCursorState& right) {
	return left.line == right.line && left.column == right.column;
}

bool PagedEditHistory::push(PagedEditRecord record, bool allowMerge) {
	if (record.before.isEmpty()) {
		return false;
	}

	// Новая правка обнуляет повтор: ветка истории стала другой.
	for (const PagedEditRecord& dropped : redoStack) {
		totalCost -= dropped.cost;
	}
	redoStack.clear();

	record.cost = costOf(record);

	if (allowMerge && !undoStack.isEmpty()) {
		PagedEditRecord& top = undoStack.last();
		if (top.mergeKind != 0 && top.mergeKind == record.mergeKind
			&& record.stamp - top.stamp <= kMergeWindowMs
			&& samePages(top, record)
			&& samePosition(top.cursorAfter, record.cursorBefore)) {
			// Набор страниц тот же: «до» уже сохранено началом набора,
			// достаточно обновить «после».
			for (const auto& entry : record.after) {
				bool replaced = false;
				for (auto& existing : top.after) {
					if (existing.first == entry.first) {
						existing.second = entry.second;
						replaced = true;
						break;
					}
				}
				if (!replaced) {
					top.after.append(entry);
				}
			}
			top.cursorAfter = record.cursorAfter;
			top.stamp = record.stamp;
			top.cost += record.cost;
			totalCost += record.cost;
			trim();
			return true;
		}
	}

	undoStack.append(record);
	totalCost += record.cost;
	trim();
	return true;
}

void PagedEditHistory::trim() {
	while (undoStack.size() > kMaxRecords
		|| (totalCost > kMaxCost && undoStack.size() > 1)) {
		totalCost -= undoStack.takeFirst().cost;
	}
}

bool PagedEditHistory::canUndo() const {
	return !undoStack.isEmpty();
}

bool PagedEditHistory::canRedo() const {
	return !redoStack.isEmpty();
}

int PagedEditHistory::undoCount() const {
	return undoStack.size();
}

bool PagedEditHistory::takeUndo(PagedEditRecord& record) {
	if (undoStack.isEmpty()) {
		return false;
	}
	record = undoStack.takeLast();
	redoStack.append(record);
	return true;
}

bool PagedEditHistory::takeRedo(PagedEditRecord& record) {
	if (redoStack.isEmpty()) {
		return false;
	}
	record = redoStack.takeLast();
	undoStack.append(record);
	return true;
}
