#include "Libs/BaseWidgets/editor/widgets/paged_text_editor/paged_text_highlighter.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin.h"
#include "Libs/BaseWidgets/editor/highlights/highlighter_plugin_manager.h"

#include <QtGlobal>

namespace {

/// Как часто сохранять состояние вложенных блоков. Чем реже, тем меньше
/// памяти на больших файлах и тем больше строк пересчитывается при прыжке.
constexpr int kStateCheckpointInterval = 512;

/// Сколько строк максимум пересчитывать назад при прыжке в неизвестное место:
/// предохраняет от зависания на файле в сотни тысяч строк.
constexpr int kMaxStateScan = 20000;

} // namespace

void PagedTextHighlighter::setPluginManager(HighlightingPluginManager* manager) {
	if (pluginManager == manager) {
		return;
	}

	pluginManager = manager;
	embeddedPlugins.clear();
}

void PagedTextHighlighter::setPlugin(HighlightingPlugin* plugin) {
	if (highlightingPlugin == plugin) {
		return;
	}

	highlightingPlugin = plugin;
	embeddedPlugins.clear();
	invalidateAll();
}

HighlightingPlugin* PagedTextHighlighter::plugin() const {
	return highlightingPlugin;
}

void PagedTextHighlighter::setLineSource(std::function<QString(int)> source) {
	lineSource = std::move(source);
	invalidateAll();
}

void PagedTextHighlighter::setLineCount(int count) {
	// Кэш состояния не сбрасывается: после правки вызывающая сторона сама
	// говорит, с какой строки он устарел (invalidateFrom), а при открытии
	// файла вызывает invalidateAll().
	lineCount = qMax(0, count);
}

void PagedTextHighlighter::invalidateFrom(int line) {
	if (!highlightingPlugin || line <= 0) {
		invalidateAll();
		return;
	}

	// Состояние перед строкой `line` остаётся верным: правка меняет только
	// её и последующие строки.
	states.erase(states.upperBound(line), states.end());
}

void PagedTextHighlighter::invalidateAll() {
	states.clear();
	states.insert(0, EmbeddedState());
}

QVector<PagedTextSpan> PagedTextHighlighter::spansForLine(const QString& line) const {
	return spansForLine(-1, line);
}

QVector<PagedTextHighlighter::EmbeddedMatch>
PagedTextHighlighter::embeddedMatches(const QString& text) const {
	QVector<EmbeddedMatch> result;
	if (!highlightingPlugin || !highlightingPlugin->hasEmbeddedRules()) {
		return result;
	}

	highlightingPlugin->visitEmbeddedMatches(text,
		[&result](qsizetype start, qsizetype length, const QTextCharFormat& format,
			int, HighlightingPlugin::HighlightingMatch::Type type,
			const QString& language) {
			result.append({ static_cast<int>(start), static_cast<int>(length),
				format,
				type == HighlightingPlugin::HighlightingMatch::Type::Start,
				language });
		});
	return result;
}

PagedTextHighlighter::EmbeddedState PagedTextHighlighter::stateAfter(
	const EmbeddedState& state, const QVector<EmbeddedMatch>& matches) const {
	EmbeddedState result = state;
	for (const EmbeddedMatch& match : matches) {
		if (match.startOfBlock) {
			result.inside = true;
			result.language = match.language;
		}
		else {
			result.inside = false;
			result.language.clear();
		}
	}
	return result;
}

PagedTextHighlighter::EmbeddedState PagedTextHighlighter::stateAt(int line) const {
	EmbeddedState state;
	if (!highlightingPlugin || !highlightingPlugin->hasEmbeddedRules()
		|| !lineSource || line <= 0) {
		return state;
	}

	const int target = qMin(line, qMax(0, lineCount));
	auto it = states.upperBound(target);
	int current = 0;
	if (it != states.begin()) {
		--it;
		current = it.key();
		state = it.value();
	}

	// Слишком далеко от известной контрольной точки — начинаем заново
	// с ограниченного окна, чтобы не пересчитывать весь файл.
	if (target - current > kMaxStateScan) {
		current = target - kMaxStateScan;
		state = EmbeddedState();
	}

	for (; current < target; current++) {
		state = stateAfter(state, embeddedMatches(lineSource(current)));
		if ((current + 1) % kStateCheckpointInterval == 0) {
			states.insert(current + 1, state);
		}
	}

	states.insert(target, state);
	return state;
}

HighlightingPlugin* PagedTextHighlighter::embeddedPlugin(
	const QString& language) const {
	if (!pluginManager || language.isEmpty()) {
		return nullptr;
	}

	const auto cached = embeddedPlugins.constFind(language);
	if (cached != embeddedPlugins.constEnd()) {
		return cached.value();
	}

	HighlightingPlugin* plugin = pluginManager->pluginForLanguage(language);
	embeddedPlugins.insert(language, plugin);
	return plugin;
}

QVector<PagedTextSpan> PagedTextHighlighter::spansForLine(int lineNumber,
	const QString& line) const {
	QVector<PagedTextSpan> spans;
	if (!highlightingPlugin || line.isEmpty()) {
		return spans;
	}

	// Длинные строки не подсвечиваются: регулярные выражения на них
	// растут квадратично и подвешивают ввод (та же граница, что в Highlighter).
	if (line.size() > Highlighter::MaxHighlightedBlockLength) {
		return spans;
	}

	const EmbeddedState state = lineNumber >= 0 ? stateAt(lineNumber)
		: EmbeddedState();
	const QVector<EmbeddedMatch> matches = embeddedMatches(line);

	// Внутри блока с заданным языком подсвечиваем его правилами; блок без
	// языка (например, HTML-тег) оставляет правила базового плагина.
	HighlightingPlugin* active = highlightingPlugin;
	if (state.inside) {
		if (HighlightingPlugin* embedded = embeddedPlugin(state.language)) {
			active = embedded;
		}
	}

	active->visitMatches(line,
		[&spans, length = line.size()](qsizetype start, qsizetype spanLength,
			const QTextCharFormat& format, int) {
			const int from = qBound(0, int(start), length);
			const int to = qBound(0, int(start + spanLength), length);
			if (to > from) {
				spans.append({ from, to - from, format });
			}
		});

	// start/end вложенного блока важнее подсветки содержимого и должны
	// применяться даже без вложенного языка.
	for (const EmbeddedMatch& match : matches) {
		const int from = qBound(0, match.startColumn, line.size());
		const int to = qBound(0, match.startColumn + match.length, line.size());
		if (to > from) {
			spans.append({ from, to - from, match.format });
		}
	}
	return spans;
}
