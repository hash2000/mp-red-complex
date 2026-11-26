#pragma once
#include <QHash>
#include <QMap>
#include <QString>
#include <QTextCharFormat>
#include <QVector>

#include <functional>

class HighlightingPlugin;
class HighlightingPluginManager;

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
///
/// Вложенные блоки (например, ``` в Markdown) требуют состояния от начала
/// документа. Состояние считается лениво по запросу строки и кэшируется
/// контрольными точками; после правки кэш сбрасывается от изменённой строки.
class PagedTextHighlighter {
public:
	void setPluginManager(HighlightingPluginManager* manager);
	void setPlugin(HighlightingPlugin* plugin);
	HighlightingPlugin* plugin() const;

	/// Источник текста строк (обычно PagedDocumentBuffer::lineAt).
	void setLineSource(std::function<QString(int)> source);
	/// Текущее число строк; кэш не сбрасывает (см. invalidateFrom).
	void setLineCount(int count);
	/// Сбросить состояние вложенных блоков, начиная со строки `line`:
	/// строки выше не изменились и остаются валидными.
	void invalidateFrom(int line);
	void invalidateAll();

	/// Подсветка строки без учёта состояния вложенных блоков (совместимый API).
	QVector<PagedTextSpan> spansForLine(const QString& line) const;
	/// Подсветка строки с учётом вложенных блоков.
	QVector<PagedTextSpan> spansForLine(int lineNumber, const QString& line) const;

private:
	struct EmbeddedState {
		bool inside = false;
		QString language;
	};

	struct EmbeddedMatch {
		int startColumn = 0;
		int length = 0;
		QTextCharFormat format;
		bool startOfBlock = false;
		QString language;
	};

	QVector<EmbeddedMatch> embeddedMatches(const QString& text) const;
	EmbeddedState stateAfter(const EmbeddedState& state,
		const QVector<EmbeddedMatch>& matches) const;
	EmbeddedState stateAt(int line) const;
	HighlightingPlugin* embeddedPlugin(const QString& language) const;

	HighlightingPluginManager* pluginManager = nullptr;
	HighlightingPlugin* highlightingPlugin = nullptr;
	std::function<QString(int)> lineSource;
	int lineCount = 0;
	/// Состояние перед строкой-ключом (контрольные точки и запрошенные строки).
	mutable QMap<int, EmbeddedState> states;
	mutable QHash<QString, HighlightingPlugin*> embeddedPlugins;
};
