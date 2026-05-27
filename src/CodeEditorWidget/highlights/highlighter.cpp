#include "CodeEditorWidget/highlights/highlighter.h"
#include <QRegularExpression>
#include <map>

namespace {
	struct HighlightingRule
	{
		QRegularExpression pattern;
		QTextCharFormat format;
		int priority = 0; // 0 = высокий, 100 = низкий
	};


	struct HighlightingMatch {
		qsizetype start;
		qsizetype length;
		QTextCharFormat format;
		int priority;
	};
}

class Highlighter::Private {
public:
	Private(Highlighter* parent) : q(parent) {}
	Highlighter* q;

	std::map<QString, HighlightingRule> rules;
	bool refreshLocked = false;
	QString previousTextCache;
	std::vector<HighlightingMatch> previusFormatCache;

	void refreshView();
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

void Highlighter::addRule(const QString& name, const QString& pattern, const QTextCharFormat& format, int priority) {
	HighlightingRule rule;
	rule.pattern = QRegularExpression(pattern);
	rule.format = format;
	rule.priority = priority;

	d->rules.emplace(name, rule);
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

void Highlighter::highlightBlock(const QString& text) {
	if (!d->previousTextCache.isEmpty() && text == d->previousTextCache && !d->previusFormatCache.empty()) {
		for (const auto& match : d->previusFormatCache) {
			setFormat(match.start, match.length, match.format);
		}
		return;
	}

	// Используем стек для хранения совпадений вместо вектора
	// если знаем что их будет немного
	std::vector<HighlightingMatch> matches;

	// Предварительно фильтруем правила: проверяем содержит ли текст
	// хотя бы одно ключевое слово из паттерна
	for (const auto& [name, rule] : d->rules) {
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

	if (matches.empty()) {
		return;
	}

	// Сортируем по начальной позиции
	std::sort(matches.begin(), matches.end(),
		[](const HighlightingMatch& a, const HighlightingMatch& b) {
		return a.start < b.start;
	});

	// Удаляем перекрытия: для каждой позиции выбираем лучшее совпадение
	d->previusFormatCache.clear();
	d->previusFormatCache.reserve(matches.size());

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
		if (d->previusFormatCache.empty() ||
			matches[bestIdx].start >= d->previusFormatCache.back().start + d->previusFormatCache.back().length) {
			d->previusFormatCache.push_back(matches[bestIdx]);
		}

		i = j;
	}

	d->previousTextCache = text;

	// Применяем финальные совпадения
	for (const auto& match : d->previusFormatCache) {
		setFormat(match.start, match.length, match.format);
	}
}

void Highlighter::lockRefreshView(bool lock) {
	d->refreshLocked = lock;
	if (!d->refreshLocked) {
		d->refreshView();
	}
}
