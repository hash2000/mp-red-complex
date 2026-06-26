#include "Content/CodeEditorWidget/highlights/plugins/highlighter_plugin.h"
#include <list>


class HighlightingPlugin::Private {
public:
	Private(HighlightingPlugin* parent) : q(parent) {}
	HighlightingPlugin* q;

	std::list<Rule> rules;
	std::list<EmbeddedBlockRule> embededBlocksRules;

	void sortFormats(std::vector<HighlightingMatch>& matches);
	std::vector<HighlightingMatch> buildFormats(const QString& text, const std::vector<HighlightingMatch>& matches);
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

QString HighlightingPlugin::Private::extractLanguage(const QRegularExpressionMatch& match, const QString& languageId) const {
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

std::vector<HighlightingPlugin::HighlightingMatch> HighlightingPlugin::matches(const QString& text) const {
	std::vector<HighlightingPlugin::HighlightingMatch> result;
	for (const auto& rule : d->rules) {
		QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
		while (it.hasNext()) {
			QRegularExpressionMatch match = it.next();
			result.push_back({
				match.capturedStart(),
				match.capturedLength(),
				rule.format,
				rule.priority,
				HighlightingMatch::Type::Start,
				QString(),
				});
		}
	}

	d->sortFormats(result);
	result = d->buildFormats(text, result);
	return std::move(result);
}

std::vector<HighlightingPlugin::HighlightingMatch> HighlightingPlugin::matchesEmbeddedBlock(const QString& text) const {
	std::vector<HighlightingPlugin::HighlightingMatch> result;
	for (const auto& rule : d->embededBlocksRules) {
		QRegularExpressionMatchIterator startIt = rule.startPattern.globalMatch(text);
		while (startIt.hasNext()) {
			QRegularExpressionMatch match = startIt.next();
			result.push_back({
				match.capturedStart(),
				match.capturedLength(),
				rule.format,
				rule.priority,
				HighlightingMatch::Type::Start,
				d->extractLanguage(match, rule.languageId),
				});
		}

		QRegularExpressionMatchIterator endMatch = rule.endPattern.globalMatch(text);
		while (endMatch.hasNext()) {
			QRegularExpressionMatch match = endMatch.next();
			result.push_back({
				match.capturedStart(),
				match.capturedLength(),
				rule.format,
				rule.priority,
				HighlightingMatch::Type::End,
				d->extractLanguage(match, rule.languageId),
				});
		}
	}

	d->sortFormats(result);
	result = d->buildFormats(text, result);
	return std::move(result);
}

void HighlightingPlugin::Private::sortFormats(std::vector<HighlightingPlugin::HighlightingMatch>& matches) {
	if (matches.empty()) {
		return;
	}

	// Сортируем по начальной позиции
	std::sort(matches.begin(), matches.end(),
		[](const HighlightingMatch& a, const HighlightingMatch& b) {
		return a.start < b.start;
	});
}

std::vector<HighlightingPlugin::HighlightingMatch> HighlightingPlugin::Private::buildFormats(const QString& text, const std::vector<HighlightingPlugin::HighlightingMatch>& matches) {
	if (matches.empty()) {
		return std::vector<HighlightingPlugin::HighlightingMatch>();
	}

	std::vector<HighlightingMatch> result;

	size_t i = 0;
	while (i < matches.size()) {
		// Находим все совпадения с одинаковым start
		size_t j = i;
		int bestIdx = i;
		while (j < matches.size() && matches[j].start == matches[i].start) {
			// Выбираем лучшее: длиннее или с более высоким приоритетом
			if (matches[j].length > matches[bestIdx].length ||
				(matches[j].length == matches[bestIdx].length && matches[j].priority < matches[bestIdx].priority)) {
				bestIdx = j;
			}
			j++;
		}

		// Добавляем только если не перекрывается с предыдущим
		if (result.empty() ||
			matches[bestIdx].start >= result.back().start + result.back().length ||
			matches[bestIdx].type <= result.back().type /* Start лучше чем End */) {
			result.push_back(matches[bestIdx]);
		}
		i = j;
	}

	return std::move(result);
}
