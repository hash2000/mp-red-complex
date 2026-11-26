#pragma once
#include <QHash>
#include <QString>

/// Кусочная загрузка очень длинных строк.
///
/// Хранит, сколько кусков уже загружено для каждой строки, и ограничивает
/// отображаемый текст. Следующий кусок запрашивается автоматически при
/// горизонтальной прокрутке — кнопки в тексте нет.
class PagedLineChunks {
public:
	static constexpr int kUnwrappedChunk = 2000;

	void clear();

	QString displayedText(const QString& lineText, int line,
		bool wrapped, int maxLineLength) const;
	int limit(int line, bool wrapped, int maxLineLength) const;
	int maximumLimit(bool wrapped, int maxLineLength) const;
	void loadMore(int line);
	/// Догружает куски, пока не станет доступна колонка `needed`.
	void ensureLimit(int line, int needed, bool wrapped, int maxLineLength);

private:
	QHash<int, int> loadedChunks;
};
