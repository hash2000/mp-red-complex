#include "CodeEditorWidget/highlights/highlighter.h"
#include "CodeEditorWidget/highlights/plugins/i_highlighter_plugin.h"
#include <QRegularExpression>
#include <map>

namespace {
	struct HighlightingRule {
		QRegularExpression pattern;
		QTextCharFormat format;
		int priority = 0; // 0 = высокий, 100 = низкий
	};

	struct HighlightingRuleEmbedded {
		QRegularExpression startPattern;
		QRegularExpression endPattern;
		QString languageId;
		QTextCharFormat format;
		int priority = 0;
	};

	struct HighlightingMatch {
		qsizetype start;
		qsizetype length;
		QTextCharFormat format;
		int priority = 0;
	};
}

class Highlighter::Private {
public:
	Private(Highlighter* parent) : q(parent) {}
	Highlighter* q;

	bool refreshLocked = false;
	QString previousTextCache;
	std::map<QString, HighlightingRule> rules;
	std::list<HighlightingRule> embeddedRules;
	std::map<QString, HighlightingRuleEmbedded> embeddedBlockRules;
	std::vector<HighlightingMatch> previusFormatCache;
	int embeddedBlockLevel = 0;

	void refreshView();
	void processRules(const QString& text, const HighlightingRule& rule, std::vector<HighlightingMatch>& matches);
	void processEmbeddedBlocksRules(const QString& text, const HighlightingRuleEmbedded& rule, std::vector<HighlightingMatch>& matches);
	QString extractLanguage(const QRegularExpressionMatch& match, const QString& languageId) const;
	void buildFormatsCache(const QString& text, std::vector<HighlightingMatch>& matches);
	void applyFormats();
};

Highlighter::Highlighter(QTextDocument* parent)
	: QSyntaxHighlighter(parent)
	, d(std::make_unique<Private>(this)) {
}

Highlighter::~Highlighter() = default;

void Highlighter::Private::refreshView() {
	if (refreshLocked) {
		return;
	}

	q->rehighlight();
}

void Highlighter::addEmbeddedCodeRule(const QString& name, const QString& startPattern, const QString& endPattern, const QString& languageId, const QTextCharFormat& format, int priority) {
	HighlightingRuleEmbedded rule;
	rule.startPattern = QRegularExpression(startPattern);
	rule.endPattern = QRegularExpression(endPattern);
	rule.languageId = languageId;
	rule.format = format;
	rule.priority = priority;

	d->embeddedBlockRules.emplace(name, rule);
	d->refreshView();
}

void Highlighter::deleteEmbeddedCodeRule(const QString& name) {
	auto it = d->embeddedBlockRules.find(name);
	if (it == d->embeddedBlockRules.end()) {
		return;
	}

	d->embeddedBlockRules.erase(it);
	d->refreshView();
}

void Highlighter::addRule(const QString& name, const QString& pattern, const QTextCharFormat& format, int priority, HighlighterRuleType type) {
	HighlightingRule rule;
	rule.pattern = QRegularExpression(pattern);
	rule.format = format;
	rule.priority = priority;

	if (type == HighlighterRuleType::Global) {
		d->rules.emplace(name, rule);
	}
	else {
		d->embeddedRules.push_back(rule);
	}

	d->refreshView();
}

void Highlighter::clearEmbeddedRules() {
	d->embeddedRules.clear();
	d->refreshView();
}

void Highlighter::deleteRule(const QString& name) {
	auto it = d->rules.find(name);
	if (it == d->rules.end()) {
		return;
	}

	d->rules.erase(it);
	d->refreshView();
}

void Highlighter::updateRulePattern(const QString& name, const QString& pattern) {
	auto it = d->rules.find(name);
	if (it == d->rules.end()) {
		return;
	}

	it->second.pattern = QRegularExpression(pattern);
	d->refreshView();
}

QString Highlighter::getRulePattern(const QString& name) const {
	auto it = d->rules.find(name);
	if (it == d->rules.end()) {
		return QString();
	}

	return it->second.pattern.pattern();
}

void Highlighter::clearRules() {
	d->rules.clear();
	d->refreshView();
}

void Highlighter::Private::processRules(const QString& text, const HighlightingRule& rule, std::vector<HighlightingMatch>& matches) {
	// Быстрая проверка: может ли правило сработать
	QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
	while (it.hasNext()) {
		QRegularExpressionMatch match = it.next();
		matches.push_back({
			match.capturedStart(),
			match.capturedLength(),
			rule.format,
			rule.priority
			});
	}
}

QString Highlighter::Private::extractLanguage(const QRegularExpressionMatch& match, const QString& languageId) const {
	if (languageId.isEmpty()) {
		return QString(); // Нет вложенного языка
	}

	// Проверяем, является ли embeddedLanguage ссылкой на группу ($1, $2, etc.)
	QRegularExpression groupRef(R"(^\$(\d+)$)");
	auto refMatch = groupRef.match(languageId);

	if (refMatch.hasMatch()) {
		// Извлекаем номер группы
		int groupNum = refMatch.captured(1).toInt();
		// Возвращаем захваченную группу из совпадения
		return match.captured(groupNum);
	}

	// Если это не ссылка на группу, возвращаем как есть (фиксированный язык)
	return languageId;
}

void Highlighter::Private::processEmbeddedBlocksRules(const QString& text, const HighlightingRuleEmbedded& rule, std::vector<HighlightingMatch>& matches) {
	auto startIt = rule.startPattern.globalMatch(text);
	while (startIt.hasNext()) {
		QRegularExpressionMatch startMatch = startIt.next();
		const auto blockStart = startMatch.capturedStart();
		const auto contentStart = startMatch.capturedEnd();
		matches.push_back({
			blockStart,
			startMatch.capturedLength(),
			rule.format,
			rule.priority
			});

		// По хорошему вложенный код может быть только один, но мало ли
		// а вот подсветка используется только от первого
		embeddedBlockLevel++;
		if (embeddedBlockLevel == 1) {
			const auto language = extractLanguage(startMatch, rule.languageId);
		}
	}

	auto endMatch = rule.endPattern.globalMatch(text);
	while (endMatch.hasNext()) {
		QRegularExpressionMatch startMatch = startIt.next();
		const auto blockStart = startMatch.capturedStart();
		const auto contentStart = startMatch.capturedEnd();
		matches.push_back({
			blockStart,
			startMatch.capturedLength(),
			rule.format,
			rule.priority
			});

		embeddedBlockLevel--;
		if (embeddedBlockLevel == 0) {
		}
	}
}

void Highlighter::Private::applyFormats() {
	// Применяем финальные совпадения
	for (const auto& match : previusFormatCache) {
		q->setFormat(match.start, match.length, match.format);
	}
}

void Highlighter::Private::buildFormatsCache(const QString& text, std::vector<HighlightingMatch>& matches) {

	if (matches.empty()) {
		return;
	}

	// Сортируем по начальной позиции
	std::sort(matches.begin(), matches.end(),
		[](const HighlightingMatch& a, const HighlightingMatch& b) {
		return a.start < b.start;
	});

	size_t i = 0;
	while (i < matches.size()) {
		// Находим все совпадения с одинаковым start
		size_t j = i;
		int bestIdx = i;
		while (j < matches.size() && matches[j].start == matches[i].start) {
			// Выбираем лучшее: длиннее или с более высоким приоритетом
			if (matches[j].length > matches[bestIdx].length ||
				(matches[j].length == matches[bestIdx].length &&
				matches[j].priority < matches[bestIdx].priority)) {
				bestIdx = j;
			}
			j++;
		}

		// Добавляем только если не перекрывается с предыдущим
		if (previusFormatCache.empty() ||
			matches[bestIdx].start >= previusFormatCache.back().start + previusFormatCache.back().length) {
			previusFormatCache.push_back(matches[bestIdx]);
		}

		i = j;
	}
}

void Highlighter::highlightBlock(const QString& text) {	
	if (!d->previousTextCache.isEmpty() && text == d->previousTextCache && !d->previusFormatCache.empty()) {
		for (const auto& match : d->previusFormatCache) {
			setFormat(match.start, match.length, match.format);
		}
		return;
	}

	const auto prevLockState = d->refreshLocked;
	d->refreshLocked = true;

	// Используем стек для хранения совпадений вместо вектора
	// если знаем что их будет немного
	std::vector<HighlightingMatch> matches;

	for (const auto& [name, rule] : d->embeddedBlockRules) {
		d->processEmbeddedBlocksRules(text, rule, matches);
	}

	if (d->embeddedBlockLevel > 0) {
		// Предварительно фильтруем правила: проверяем содержит ли текст
		// хотя бы одно ключевое слово из паттерна
		for (const auto& [name, rule] : d->rules) {
			d->processRules(text, rule, matches);
		}

	}
	else {
		for (const auto& rule : d->embeddedRules) {
			d->processRules(text, rule, matches);
		}
	}

	d->previusFormatCache.clear();
	d->previousTextCache = text;
	d->buildFormatsCache(text, matches);
	d->previusFormatCache.reserve(matches.size());
	d->applyFormats();

	d->refreshLocked = prevLockState;
}

void Highlighter::lockRefreshView(bool lock) {
	d->refreshLocked = lock;
	if (!d->refreshLocked) {
		d->refreshView();
	}
}
