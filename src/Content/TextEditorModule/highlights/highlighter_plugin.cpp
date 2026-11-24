#include "Content/TextEditorModule/highlights/highlighter_plugin.h"

#include <algorithm>
#include <list>

namespace {
/// Кандидат подсветки до выбора перекрывающихся фрагментов.
///
/// Формат хранится указателем: правила после install() не меняются,
/// а копирование QTextCharFormat на каждый фрагмент заметно на больших текстах.
struct Candidate {
	qsizetype start = 0;
	qsizetype length = 0;
	const QTextCharFormat* format = nullptr;
	int priority = 0;
	HighlightingPlugin::HighlightingMatch::Type type =
		HighlightingPlugin::HighlightingMatch::Type::Start;
	QString language;
};
}

class HighlightingPlugin::Private {
public:
	Private(HighlightingPlugin* parent) : q(parent) {}
	HighlightingPlugin* q;

	std::list<Rule> rules;
	std::list<EmbeddedBlockRule> embededBlocksRules;

	void collectRules(const QString& text, std::vector<Candidate>& out) const;
	void collectEmbedded(const QString& text, std::vector<Candidate>& out) const;
	static void buildFormats(std::vector<Candidate>& candidates);
	static void collectMatches(const std::vector<Candidate>& candidates,
		std::vector<HighlightingPlugin::HighlightingMatch>& out);
	QString extractLanguage(const QRegularExpressionMatch& match, const QString& languageId) const;
};

HighlightingPlugin::HighlightingPlugin()
	: d(std::make_unique<Private>(this)) {
}

HighlightingPlugin::~HighlightingPlugin() = default;

void HighlightingPlugin::addRule(const Rule& rule) {
	d->rules.push_back(rule);
}

void HighlightingPlugin::addEmbeddedBlockRule(const EmbeddedBlockRule& rule) {
	d->embededBlocksRules.push_back(rule);
}

bool HighlightingPlugin::hasEmbeddedRules() const {
	return !d->embededBlocksRules.empty();
}

bool HighlightingPlugin::hasRules() const {
	return !d->rules.empty();
}

QString HighlightingPlugin::Private::extractLanguage(const QRegularExpressionMatch& match,
	const QString& languageId) const {
	if (languageId.isEmpty()) {
		return QString(); // Нет вложенного языка
	}

	// Проверяем, является ли embeddedLanguage ссылкой на группу ($1, $2, etc.)
	static const QRegularExpression groupRef(R"(^\$(\d+)$)");
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

void HighlightingPlugin::Private::collectRules(const QString& text,
	std::vector<Candidate>& out) const {
	for (const auto& rule : rules) {
		if (!rule.pattern.isValid()) {
			continue;
		}

		QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
		while (it.hasNext()) {
			const QRegularExpressionMatch match = it.next();
			out.push_back({
				match.capturedStart(),
				match.capturedLength(),
				&rule.format,
				rule.priority,
				HighlightingMatch::Type::Start,
				QString(),
				});
		}
	}
}

void HighlightingPlugin::Private::collectEmbedded(const QString& text,
	std::vector<Candidate>& out) const {
	for (const auto& rule : embededBlocksRules) {
		if (!rule.startPattern.isValid() || !rule.endPattern.isValid()) {
			continue;
		}

		QRegularExpressionMatchIterator startIt = rule.startPattern.globalMatch(text);
		while (startIt.hasNext()) {
			const QRegularExpressionMatch match = startIt.next();
			out.push_back({
				match.capturedStart(),
				match.capturedLength(),
				&rule.format,
				rule.priority,
				HighlightingMatch::Type::Start,
				extractLanguage(match, rule.languageId),
				});
		}

		QRegularExpressionMatchIterator endIt = rule.endPattern.globalMatch(text);
		while (endIt.hasNext()) {
			const QRegularExpressionMatch match = endIt.next();
			out.push_back({
				match.capturedStart(),
				match.capturedLength(),
				&rule.format,
				rule.priority,
				HighlightingMatch::Type::End,
				extractLanguage(match, rule.languageId),
				});
		}
	}
}

void HighlightingPlugin::Private::buildFormats(std::vector<Candidate>& candidates) {
	if (candidates.empty()) {
		return;
	}

	// Сортируем по начальной позиции
	std::sort(candidates.begin(), candidates.end(),
		[](const Candidate& a, const Candidate& b) {
			return a.start < b.start;
		});

	// Для одинакового start выбирается более длинный фрагмент, при равной
	// длине — с более высоким приоритетом (меньшим числом). Перекрывающиеся
	// фрагменты отбрасываются, кроме случая Start поверх End: так начало
	// вложенного блока важнее его конца.
	std::vector<Candidate> kept;
	kept.reserve(candidates.size());

	size_t i = 0;
	while (i < candidates.size()) {
		size_t j = i;
		size_t bestIdx = i;
		while (j < candidates.size() && candidates[j].start == candidates[i].start) {
			if (candidates[j].length > candidates[bestIdx].length ||
				(candidates[j].length == candidates[bestIdx].length &&
					candidates[j].priority < candidates[bestIdx].priority)) {
				bestIdx = j;
			}
			j++;
		}

		const Candidate& best = candidates[bestIdx];
		if (best.format &&
			(kept.empty() ||
				best.start >= kept.back().start + kept.back().length ||
				best.type <= kept.back().type)) {
			kept.push_back(best);
		}
		i = j;
	}

	candidates.swap(kept);
}

void HighlightingPlugin::Private::collectMatches(const std::vector<Candidate>& candidates,
	std::vector<HighlightingPlugin::HighlightingMatch>& out) {
	out.reserve(candidates.size());
	for (const auto& candidate : candidates) {
		if (!candidate.format) {
			continue;
		}

		out.push_back({
			candidate.start,
			candidate.length,
			*candidate.format,
			candidate.priority,
			candidate.type,
			candidate.language,
			});
	}
}

void HighlightingPlugin::visitMatches(const QString& text, const MatchVisitor& visitor) const {
	if (d->rules.empty()) {
		return;
	}

	std::vector<Candidate> candidates;
	d->collectRules(text, candidates);
	Private::buildFormats(candidates);
	for (const auto& candidate : candidates) {
		visitor(candidate.start, candidate.length, *candidate.format, candidate.priority);
	}
}

void HighlightingPlugin::visitEmbeddedMatches(const QString& text,
	const EmbeddedMatchVisitor& visitor) const {
	if (d->embededBlocksRules.empty()) {
		return;
	}

	std::vector<Candidate> candidates;
	d->collectEmbedded(text, candidates);
	Private::buildFormats(candidates);
	for (const auto& candidate : candidates) {
		visitor(candidate.start, candidate.length, *candidate.format, candidate.priority,
			candidate.type, candidate.language);
	}
}

std::vector<HighlightingPlugin::HighlightingMatch> HighlightingPlugin::matches(
	const QString& text) const {
	std::vector<Candidate> candidates;
	d->collectRules(text, candidates);
	Private::buildFormats(candidates);

	std::vector<HighlightingMatch> result;
	d->collectMatches(candidates, result);
	return result;
}

std::vector<HighlightingPlugin::HighlightingMatch> HighlightingPlugin::matchesEmbeddedBlock(
	const QString& text) const {
	std::vector<Candidate> candidates;
	d->collectEmbedded(text, candidates);
	Private::buildFormats(candidates);

	std::vector<HighlightingMatch> result;
	d->collectMatches(candidates, result);
	return result;
}
